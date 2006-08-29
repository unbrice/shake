/* Copyright (C) 2006 Brice Arnould.
 *  This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include <stdlib.h>
#include <stdio.h>		// snprintf
#include <limits.h>		// CHAR_BIT
#include <time.h>		// time, time_t
#include <assert.h>		// assert
#include <errno.h>		// errno
#include <error.h>		// error()
#include <attr/attributes.h>	// attr_setf,
#include <sys/ioctl.h>		// ioctl()
#include <linux/fs.h>		// FIBMAP, FIGETBSZ
#include <arpa/inet.h>		// htonl, ntohl
#include "linux.h"

/*  could make an estimation of the required size, but should'nt because attr_setf
 * set fixed size attributes, so it would cause problems when moving the disk
 */
#define DATE_SIZE sizeof(uint32_t)	// TODO: change this value before 2107


/* Set the shake_ptime field and ctime of the file to the actual date.
 */
int
set_ptime (int fd)
{
  assert (fd > -1);
  uint32_t date = htonl ((uint32_t) time (NULL));
  int size = DATE_SIZE;
  return attr_setf (fd, "shake.ptime", (char *) &date, size, ATTR_DONTFOLLOW);
}

/* Get the ptime of the file, -1 if that failed
 */
time_t
get_ptime (int fd)
{
  assert (fd > -1);
  uint32_t date;
  int size = DATE_SIZE;
  if (-1 ==
      attr_getf (fd, "shake.ptime", (char *) &date, &size, ATTR_DONTFOLLOW))
    return (time_t) - 1;
  date = ntohl (date);
  if (date > time (NULL))
    return (time_t) - 1;
  return (time_t) date;
}


/*  This function is mainly a wrapper around ioctl()s.
 *  It updates a->{blocks, crumbc, fragc, start and end}
 * with just a bit of undocumented black magic.
 *  It can only be called after investigate().
 */
int
get_testimony (struct accused *a, struct law *l)
{
  const size_t BUFFSTEP = 32;
  /* General stats */
  int physbsize;
  int crumbsize;
  /* Framents logs */
  llint *sizelog = NULL, *poslog = NULL; // Framgents sizes and positions
  int logs_pos = 0;		// Position in logs
  /* Convert sizes in number of physical blocks */
  {
    if (-1 == ioctl (a->fd, FIGETBSZ, &physbsize))
      {
	error (0, errno, "%s: FIGETBSZ() failed", a->name);
	return -1;
      }
    a->blocks = (a->size + physbsize - 1) / physbsize;
    crumbsize = a->size * l->crumbratio;
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
	  break;		// The file is too large for FIBMAP
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
	fragsize +=  physbsize;
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
