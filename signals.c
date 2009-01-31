/* Copyright (C) 2006-2008 Brice Arnould.
 *  This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/* This file contains everything that is not thread-safe, that is
 * mode-handling 
 */

#include "signals.h"
#include "linux.h"		//
#include <assert.h>		// assert()
#include <errno.h>		// errno
#include <error.h>		// error()
#include <signal.h>		// sigaction, sigprocmask, sigsetops
#include <unistd.h>		// unlink()
#include <stdbool.h>

/* Those variables would have to be put in a thread-specific storage.
 */
static const char *current_msg = NULL;
static const char *current_tempfile = NULL;
static const char *current_file = NULL;	// The file being shaked
static enum mode current_mode;	// Tell in which mode we are, cf signals.h

/*  If we're in REWRITE mode, display current_msg and exit,
 * if we're in BACKUP mode, cancel the backup by going in cancel mode
 * else unlink the current_tempfile and call he default handler
 */
static void
handle_signals (int sig)
{
  if (current_mode == REWRITE && sig == SIGLOCKEXPIRED)
    {
      error (0, 0,
	     "%s: Another program is trying to access the file; "
	     "if shaking takes more than lease-break-time seconds "
	     "shake will be killed; if this happens a backup will be "
	     "available in '%s'", current_file, current_tempfile);
    }
  else if (current_mode == REWRITE)
    {
      /* Appart from SIGLOCKEXPIRED, we receive only SIGILL, SIGFPE or
       * SIGSEG in this mode (that is, fatal signals) */
      assert (current_msg);
      error (1, 0, "%s", current_msg);
    }
  else if (current_mode == BACKUP && sig == SIGLOCKEXPIRED)
    {
      assert (current_file);
      error (0, 0, "Failed to shake: %s: concurent accesses", current_file);
      current_mode = CANCEL;
    }
  else
    {
      assert (current_tempfile);
      unlink (current_tempfile);
      raise (sig);		// Call the default handler, because
      // sa_flags == SA_RESETHAND
    }
}

void
install_sighandler (const char *tempfile)
{
  assert (tempfile);
  struct sigaction sa;
  current_tempfile = tempfile;
  sigemptyset (&sa.sa_mask);
  sa.sa_flags = (int) SA_RESETHAND;	// All signals after the firsts will be
  // handled by system's default handlers
  sa.sa_handler = handle_signals;
  /* Set our handler as the one that will handle critical situations */
  sigaction (SIGFPE, &sa, NULL);
  sigaction (SIGHUP, &sa, NULL);
  sigaction (SIGILL, &sa, NULL);
  sigaction (SIGINT, &sa, NULL);
  sigaction (SIGKILL, &sa, NULL);
  sigaction (SIGLOCKEXPIRED, &sa, NULL);
  sigaction (SIGPIPE, &sa, NULL);
  sigaction (SIGQUIT, &sa, NULL);
  sigaction (SIGSEGV, &sa, NULL);
  sigaction (SIGTERM, &sa, NULL);
  sigaction (SIGUSR1, &sa, NULL);
  sigaction (SIGUSR2, &sa, NULL);
  sigaction (SIGBUS, &sa, NULL);
  sigaction (SIGXCPU, &sa, NULL);
  sigaction (SIGXFSZ, &sa, NULL);
  /* Set the NORMAL mode */
  enter_normal_mode ();
}

void
enter_normal_mode (void)
{
  sigset_t sset;
  sigfillset (&sset);
  current_mode = NORMAL;
  current_msg = NULL;
  current_file = NULL;
  sigprocmask (SIG_UNBLOCK, &sset, NULL);
}

void
enter_backup_mode (const char *filename)
{
  assert (current_mode == NORMAL);
  current_mode = BACKUP;
  current_file = filename;
}


void
enter_critical_mode (const char *msg)
{
  assert (msg), assert (current_mode == BACKUP);
  sigset_t sset;
  sigfillset (&sset);
  current_msg = msg;
  current_mode = REWRITE;
  /* Don't suspend signals raised by internal errors */
  sigdelset (&sset, SIGILL);
  sigdelset (&sset, SIGFPE);
  sigdelset (&sset, SIGSEGV);
  /* Don't suspend signals raised by lock contention */
  sigdelset (&sset, SIGLOCKEXPIRED);
  /* Stop and suspend works as usual */
  sigdelset (&sset, SIGTSTP);
  sigdelset (&sset, SIGSTOP);
  sigprocmask (SIG_BLOCK, &sset, NULL);
}

enum mode
get_current_mode (void)
{
  return current_mode;
}
