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

#ifndef LINUX_H
# define LINUX_H
# define _GNU_SOURCE
# include <getopt.h>		// getopt_long()
# include <alloca.h>		// alloca()
# include <stdbool.h>		// bool
# include <stdio.h>		// getline, asprintf
# include <sys/time.h>		// struct timeval
# include "judge.h"

# define OS_RESERVED_SIGNAL 16

/* Called once, perform OS-specific tasks.
 */
int os_specific_setup (const char *tempfile);



/* Get a write lock on the file.
 * We get a write lock even when a read lock would be enough to detect
 * earlier access contention.
 */
int readlock_file (int fd, const char *filename);

/* Release our locks on the file
 * Return -1 if we do not own the lock.
 */
int unlock_file (int fd);

/* Promote a read lock to a write lock.
 * Return -1 if we no longer owned the lock.
 */
int readlock_to_writelock (int fd);

/* Return true if fd is locked, else false
 */
bool is_locked (int fd);



/* Declares the glibc function
 */
int futimes (int fd, const struct timeval tv[2]);

/* Set the shake_ptime field and ctime of the file to the actual date.
 */
int set_ptime (int fd);

/* Get the ptime of the file, -1 if that failed
 */
time_t get_ptime (int fd);

/*  This function is mainly a wrapper around ioctl()s.
 *  It updates a->{blocks, crumbc, fragc, start and end}
 * with just a bit of undocumented black magic.
 *  It can only be called after investigate().
 */
int get_testimony (struct accused *a, struct law *l);

#endif
