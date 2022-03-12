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

#include "linux.h"

#include <stdlib.h>
#include <stdio.h>              // snprintf
#include <limits.h>             // CHAR_BIT
#include <time.h>               // time, time_t
#include <assert.h>             // assert
#include <errno.h>              // errno
#include <error.h>              // error()
#include <fcntl.h>              // fcntl()
#include <signal.h>             // sigaction()
#include <unistd.h>             // fcntl()
#include <sys/ioctl.h>          // ioctl()
#include <sys/xattr.h>          // fgetxattr(), setxattr()
#include <linux/fs.h>           // FIBMAP, FIGETBSZ
#include <arpa/inet.h>          // htonl, ntohl

/* The following try to hide Linux-specific leases behind an interface
 * similar to Posix locks.
 * It does a lot of thread-unsafe black magic.
 */

#define SIGLOCKEXPIRED OS_RESERVED_SIGNAL
#define MAX_LOCKED_FDS 2        // Never greater than 1

/* Describe locks
 */
struct lock_desc
{
  const char *filename;
  int fd;
  bool write;
};

/* All the currently managed locks (thread-unsafe, would require a mutex)
 */
struct lock_desc LOCKS[MAX_LOCKED_FDS];

/* The current temporary file
 */
const char *TEMPFILE;

/* Return the position of the given fd in LOCKS or a position for the
 * invalid fd ( -1 ) if there is no such position
 * We are sure it exists because there can only be one locked file
 * and LOCKS has a size of 2.
 */
static int
locate_lock (int searchedfd)
{
  int invalidfd_pos = -1;
  for (int i = 0; i < MAX_LOCKED_FDS; i++)
    if (LOCKS[i].fd == searchedfd)
      return i;
    else if (LOCKS[i].fd == -1)
      invalidfd_pos = i;
  assert (invalidfd_pos >= 0);
  return invalidfd_pos;
}

/* Called when a lease is being cancelled
 */
static void
handle_broken_locks (int sig, siginfo_t * info, void *ignored)
{
  assert (SIGLOCKEXPIRED == sig), assert (ignored);
  int fd = info->si_fd;
  int pos = locate_lock (fd);
  assert (LOCKS[pos].fd != -1);
  if (LOCKS[pos].write)
    error (0, 0,
           "%s: Another program is trying to access the file; "
           "if shaking takes more than lease-break-time seconds "
           "shake will be killed; if this happens a backup will be "
           "available in '%s'", LOCKS[pos].filename, TEMPFILE);
  else
    {
      // Cancel this lock
      LOCKS[pos].fd = -1;
      error (0, 0, "%s: concurent accesses", LOCKS[pos].filename);
    }
}

int
os_specific_setup (const char *tempfile)
{
  /* Initialize globals */
  TEMPFILE = tempfile;
  for (int i = 0; i < MAX_LOCKED_FDS; i++)
    LOCKS[i].fd = -1;
  /* Setup SIGLOCKEXPIRED handler */
  struct sigaction sa;
  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = handle_broken_locks;
  return sigaction (SIGLOCKEXPIRED, &sa, NULL);
}

int
readlock_file (int fd, const char *filename)
{
  int pos = locate_lock (fd);
  assert (LOCKS[pos].fd == -1);
  // Technically all our locks are write leases
  if (fcntl (fd, F_SETLEASE, F_WRLCK) != 0)
    return -1;
  if (fcntl (fd, F_SETSIG, SIGLOCKEXPIRED) != 0)
    return -1;
  /* Register the lock in LOCKS */
  {
    LOCKS[pos].filename = filename;
    LOCKS[pos].fd = fd;
    LOCKS[pos].write = false;
  }
  return 0;
}

int
readlock_to_writelock (int fd)
{
  int pos = locate_lock (fd);
  if (0 > LOCKS[pos].fd)
    return -1;                  // The lock has been canceled
  LOCKS[pos].write = true;
  return 0;
}

int
unlock_file (int fd)
{
  int pos = locate_lock (fd);
  if (0 > LOCKS[pos].fd)
    return -1;
  LOCKS[pos].fd = -1;
  // TODO(unbrice): This line is so as to help debugging unlock_file()
  // remove it in a few months.
  errno = 0;
  return fcntl (fd, F_SETLEASE, F_UNLCK);
}

bool
is_locked (int fd)
{
  return LOCKS[locate_lock (fd)].fd >= 0;
}

/*  could make an estimation of the required size, but should'nt because attr_setf
 * set fixed size attributes, so it would cause problems when moving the disk
 */
#define DATE_SIZE sizeof(uint32_t)      // TODO: change this value before 2107

int
set_ptime (int fd)
{
  assert (fd > -1);
  uint32_t date = htonl ((uint32_t) time (NULL));
  return fsetxattr (fd, "shake.ptime", (void *) &date, DATE_SIZE, 0);
}

time_t
get_ptime (int fd)
{
  assert (fd > -1);
  uint32_t date;
  int size = DATE_SIZE;
  if (-1 ==
      fgetxattr (fd, "shake.ptime", (void *) &date, size))
    return (time_t) - 1;
  date = ntohl (date);
  if (date > time (NULL))
    return (time_t) - 1;
  return (time_t) date;
}

int
get_testimony (struct accused *a, struct law *l)
{
  const size_t BUFFSTEP = 32;
  /* General stats */
  uint physbsize;
  int crumbsize;
  /* Framents logs */
  llint *sizelog = NULL, *poslog = NULL;        // Framgents sizes and positions
  unsigned int logs_pos = 0;    // Position in logs
  /* Convert sizes in number of physical blocks */
  {
    if (-1 == ioctl (a->fd, FIGETBSZ, &physbsize))
      {
        error (0, errno, "%s: FIGETBSZ() failed", a->name);
        return -1;
      }
    a->blocks = (a->size + physbsize - 1) / physbsize;
    crumbsize = (int) ((double) a->size * l->crumbratio);
  }
  /* Create the log of fragment, terminated by <-1,-1> */
  if (l->verbosity >= 3)
    {
      sizelog = malloc (BUFFSTEP * sizeof (*sizelog));
      poslog = malloc (BUFFSTEP * sizeof (*poslog));
      if (!sizelog || !poslog)
        error (1, errno, "%s: malloc() failed", a->name);
      sizelog[logs_pos] = -1;
      poslog[logs_pos] = -1;
    }
  /*  FIBMAP return physical block's position. We use it to detect start and end
   * of fragments, by checking if the physical position of a block is not
   * adjacent to the previous one.
   *  Please refer to comp.os.linux.development.system for information about
   * FIBMAP (or ask me but I don't know anything which is not in this file).
   */
  {
    llint physpos = 0, prevphyspos = 0;
    uint fragsize = 0;
    for (int i = 0; i < a->blocks; i++)
      {
        if (INT_MAX == i)
          break;                // The file is too large for FIBMAP
        /* Query the physical pos of the i-nth block */
        prevphyspos = physpos;
        physpos = i;
        if (-1 == ioctl (a->fd, FIBMAP, &physpos))
          {
            error (0, errno, "%s: FIBMAP failed", a->name);
            return -1;
          }
        physpos = physpos * physbsize;
        /* workaround reiser4 bug fixed 2006-08-27, TODO : remove */
        if (physpos < 0)
          {
            error (0, 0, "ReiserFS4 bug : UPDATE to at least 2006-08-27");
            physpos = 0;
          }
        /* physpos == 0 if sparse file */
        if (physpos)
          {
            if (!a->start)
              a->start = physpos;
            a->end = physpos;
            /* Check if we have a new fragment, */
            if (llabs (physpos - prevphyspos) > MAGICLEAP)
              {
                /* log it */
                if (l->verbosity >= 3)
                  {
                    /* Periodically enlarge the log */
                    if (0 == (logs_pos + 2) % BUFFSTEP)
                      {
                        size_t nsize =
                          (logs_pos + 2 + BUFFSTEP) * sizeof (*sizelog);
                        sizelog = realloc (sizelog, nsize);
                        poslog = realloc (poslog, nsize);
                        if (!sizelog || !poslog)
                          error (1, errno, "%s: malloc() failed", a->name);
                      }
                    /* Record the pos of the new frag */
                    poslog[logs_pos] = physpos;
                    /* Record the size of the old frag */
                    if (logs_pos)
                      sizelog[logs_pos - 1] = fragsize;
                    logs_pos++;
                  }
                if (fragsize && fragsize < crumbsize)
                  a->crumbc++;
                a->fragc++;
                fragsize = 0;
              }
          }
        fragsize += physbsize;
      }
    /* Record the last size, and close the log */
    if (l->verbosity >= 3 && fragsize)
      {
        if (logs_pos)
          sizelog[logs_pos - 1] = fragsize;
        poslog[logs_pos] = -1;
        sizelog[logs_pos] = -1;
        a->poslog = poslog;
        a->sizelog = sizelog;
      }
  }
  return 0;
}
