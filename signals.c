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

/* This file contains everything that is not thread-safe
 */

#include <assert.h>		// assert()
#include <errno.h>		// errno
#include <error.h>		// error()
#include <signal.h>		// sigaction, sigprocmask, sigsetops
#include <unistd.h>		// unlink()
#include <stdbool.h>

#pragma warning Everything in this file is not thread safe
static const char *current_msg = NULL;
static const char *current_tempfile = NULL;
static bool critical_mode = false;	// Tell in which mode we are, please see
				        // restrict_signals and restore_signals

/*  If we're in critical_mode, display current_msg and exit, else
 * unlink the current_tempfile and call he default handler
 */
static void
handle_signals (int sig)
{
  if (critical_mode)
    {
      /* We recieve only SIGILL, SIGFPE or SIGSEG in this mode */
      assert (current_msg);
      error (1, 0, current_msg);
    }
  else
    {
      assert (current_tempfile);
      unlink (current_tempfile);
      raise (sig);		// Call the default handler, because sa_flags == SA_RESETHAND
    }
}

/*  Set handle_signals() as the default handler, and tempfile
 * as the current temporary file
 */
void
install_sighandler (const char *tempfile)
{
  assert (tempfile);
  struct sigaction sa;
  current_tempfile = tempfile;
  sigemptyset (&sa.sa_mask);
  sa.sa_flags = SA_RESETHAND;	// All signals after the firsts will be
                                // handled by system's default handlers
  sa.sa_handler = handle_signals;
  /* Set our handler as the one that will handle critical situations */
  sigaction (SIGFPE, &sa, NULL);
  sigaction (SIGHUP, &sa, NULL);
  sigaction (SIGILL, &sa, NULL);
  sigaction (SIGINT, &sa, NULL);
  sigaction (SIGKILL, &sa, NULL);
  sigaction (SIGPIPE, &sa, NULL);
  sigaction (SIGQUIT, &sa, NULL);
  sigaction (SIGSEGV, &sa, NULL);
  sigaction (SIGTERM, &sa, NULL);
  sigaction (SIGUSR1, &sa, NULL);
  sigaction (SIGUSR2, &sa, NULL);
  sigaction (SIGBUS, &sa, NULL);
  sigaction (SIGXCPU, &sa, NULL);
  sigaction (SIGXFSZ, &sa, NULL);
}

/* In this mode :
 *  signals raised by internal errors, such as SIGSEV, stop the program
 * and show informations about the error (char *msg)
 *  signals that suspend the activity works as usual
 *  others are suspended
 * It is intended to be used when current_tempfile is the only copy of a file
 */
void
restrict_signals (const char *msg)
{
  assert (msg);
  sigset_t sset;
  sigfillset (&sset);
  current_msg = msg;
  critical_mode = true;
  /* Don't suspend signals raised by internal errors */
  sigdelset (&sset, SIGILL);
  sigdelset (&sset, SIGFPE);
  sigdelset (&sset, SIGSEGV);
  /* Stop and suspend works as usual */
  sigdelset (&sset, SIGTSTP);
  sigdelset (&sset, SIGSTOP);
  sigprocmask (SIG_BLOCK, &sset, NULL);
}

/*  In this mode, signals are handled with the default handler, appart
 * that the temporary file will be deleted.
 */
void
restore_signals (void)
{
  sigset_t sset;
  sigfillset (&sset);
  critical_mode = false;
  current_msg = NULL;
  sigprocmask (SIG_UNBLOCK, &sset, NULL);
}
