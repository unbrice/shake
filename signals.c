/***************************************************************************/
/*  Copyright (C) 2006-2011 Brice Arnould.                                 */
/*                                                                         */
/*  This file is part of ShaKe.                                            */
/*                                                                         */
/*  ShaKe is free software; you can redistribute it and/or modify          */
/*  it under the terms of the GNU General Public License as published by   */
/*  the Free Software Foundation; either version 3 of the License, or      */
/*  (at your option) any later version.                                    */
/*                                                                         */
/*  This program is distributed in the hope that it will be useful,        */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of         */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          */
/*  GNU General Public License for more details.                           */
/*                                                                         */
/*  You should have received a copy of the GNU General Public License      */
/*  along with this program.  If not, see <http://www.gnu.org/licenses/>.  */
/***************************************************************************/

#include "linux.h"		// OS_RESERVED_SIGNAL
#include "signals.h"
#include <assert.h>		// assert()
#include <errno.h>		// errno
#include <error.h>		// error()
#include <signal.h>		// sigaction, sigprocmask, sigsetops
#include <unistd.h>		// unlink()
#include <stdbool.h>

/* Those variables would have to be put in a thread-specific storage
 * for ShaKe to be multithread
 */
static const char *current_msg = NULL;
static const char *current_tempfile = NULL;
static const char *current_file = NULL;	// The file being shaked
static volatile enum mode current_mode;	// Tell in which mode we are, cf signals.h

/*  If we're in CRITICAL mode, display current_msg and exit,
 * if we're in PREPARE mode, cancel the backup by going in cancel mode
 * else unlink the current_tempfile and call he default handler
 */
static void
handle_signals (int sig)
{
  if (current_mode == CRITICAL)
    {
      /* Appart from SIGLOCKEXPIRED, we receive only SIGILL, SIGFPE or
       * SIGSEG in this mode (that is, fatal signals) */
      assert (current_msg);
      error (1, 0, "%s", current_msg);
    }
  else
    {
      assert (current_tempfile);
      unlink (current_tempfile);
      // Calls the default handler, because sa_flags == SA_RESETHAND
      raise (sig);
    }
}

/* Does what sigaction() would, except if the previous
 * handler is SIG_IGN in which case it does nothing.
 */
static int
sigaction_or_ignore (int signum, const struct sigaction *action,
		     struct sigaction *old_action_res)
{
  struct sigaction old_action;
  if (-1 == sigaction (signum, NULL, &old_action))
    return -1;
  else if (SIG_IGN == old_action.sa_handler)
    {
      /* Signal was set to "ignore", do not change it,
       * but set old_action_res if it is not NULL
       */
      if (NULL != old_action_res)
	*old_action_res = old_action;
      return 0;
    }
  else
    return sigaction (signum, action, old_action_res);
}


void
install_sighandler (const char *tempfile)
{
  assert (tempfile);
  struct sigaction sa;
  current_tempfile = tempfile;
  sigemptyset (&sa.sa_mask);
  // All signals after the firsts will be handled by system's default
  // handlers
  sa.sa_flags = SA_RESETHAND;
  // Automatically restart syscalls after handling a signal
  sa.sa_flags |= SA_RESTART;
  sa.sa_handler = handle_signals;
  /* Set our handler as the one that will handle critical situations */
  sigaction (SIGFPE, &sa, NULL);
  sigaction (SIGILL, &sa, NULL);
  sigaction (SIGPIPE, &sa, NULL);
  sigaction (SIGSEGV, &sa, NULL);
  sigaction (SIGTERM, &sa, NULL);
  sigaction (SIGUSR1, &sa, NULL);
  sigaction (SIGUSR2, &sa, NULL);
  sigaction (SIGBUS, &sa, NULL);
  sigaction (SIGXCPU, &sa, NULL);
  sigaction (SIGXFSZ, &sa, NULL);
  /* The same holds terminal-related signals, except that we need to
   * take in account the shell's dispositions.
   */
  sigaction_or_ignore (SIGHUP, &sa, NULL);
  sigaction_or_ignore (SIGINT, &sa, NULL);
  sigaction_or_ignore (SIGQUIT, &sa, NULL);
  sigaction_or_ignore (SIGTTIN, &sa, NULL);
  sigaction_or_ignore (SIGTTOU, &sa, NULL);
  /* Set the NORMAL mode */
  enter_normal_mode ();
}

void
enter_normal_mode (void)
{
  sigset_t sset;
  assert (NORMAL != current_mode);
  sigfillset (&sset);
  current_mode = NORMAL;
  current_msg = NULL;
  current_file = NULL;
  sigprocmask (SIG_UNBLOCK, &sset, NULL);
}

void
enter_critical_mode (const char *msg)
{
  sigset_t sset;
  assert (CRITICAL != current_mode);
  sigfillset (&sset);
  current_msg = msg;
  /* Don't suspend signals raised by internal errors */
  sigdelset (&sset, SIGILL);
  sigdelset (&sset, SIGFPE);
  sigdelset (&sset, SIGSEGV);
  /* Don't suspend signals reserved for use by the OS-specific layer */
  sigdelset (&sset, OS_RESERVED_SIGNAL);
  /* Stop and suspend works as usual */
  sigdelset (&sset, SIGTSTP);
  sigdelset (&sset, SIGSTOP);
  sigprocmask (SIG_BLOCK, &sset, NULL);
  current_mode = CRITICAL;
}
