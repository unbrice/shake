/***************************************************************************/
/*  Copyright (C) 2006-2009 Brice Arnould.                                 */
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
#include "executive.h"
#include "signals.h"
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <linux/fs.h>		// FIGETBSZ
#include <limits.h>		// SSIZE_MAX
#include <sys/stat.h>		// stat()
#include <unistd.h>		// stat()
#include <sys/ioctl.h>		// ioctl()
#include <error.h>		// error()
#include <sys/types.h>		// opendir()
#include <dirent.h>		// opendir()
#include <sys/time.h>		// futimes()


int
fcopy (int in_fd, int out_fd, size_t gap)
{
  assert (in_fd > -1), assert (out_fd > -1);
  size_t buffsize = 65535;	// Must fit in a integer
  int *buffer;
  /* Prepare files */
  if (-1 == lseek (in_fd, (off_t) 0, SEEK_SET)
      || -1 == lseek (out_fd, (off_t) 0, SEEK_SET)
      || -1 == ftruncate (out_fd, (off_t) 0))
    return -1;
  /* Optimisation (on Linux it double the readahead window) */
  posix_fadvise (in_fd, (off_t) 0, (off_t) 0, POSIX_FADV_SEQUENTIAL);
  posix_fadvise (in_fd, (off_t) 0, (off_t) 0, POSIX_FADV_WILLNEED);
  /* Get a buffer... */
  {
    if (gap)
      {
	int physbsize;
	/*  Convert the gap in a number of blocks
	 *  The idea is that it would be useless to make only a part of a block
	 * sparse, so we use buffers of physical block size and make a hole
	 * only if there's enough consecutive empty buffers.
	 */
	if (-1 == ioctl (out_fd, FIGETBSZ, &physbsize))
	  return -1;
	if (gap >= physbsize)
	  {
	    gap /= physbsize;
	    // now gap is number of empty buffers required to make the file sparse
	    buffsize = (size_t) physbsize;
	  }
      }
    if (buffsize > SSIZE_MAX)
      {
	buffsize = SSIZE_MAX;
	gap = 0;
      }
    buffer = alloca (buffsize);	// better than "goto freeall"... or not ?
    // Else would read uninitialised datas when filesize < buffsize
    // and is_empty could be set uncorrectly.
    memset (buffer, 0xFF, buffsize);
  }
  /* Let's go ! */
  {
    int len = 0;
    uint empty_buffs = 0;	// Number of consecutive empty buffers, for sparse files
    bool is_empty = 0;		// Tell if the buffer is empty, for sparse files
    int *empty = NULL;		// An empty buffer, for sparse files
    if (gap)
      {
	empty = alloca (buffsize);	// better than goto free()... or not ?
	if (!empty)
	  return -1;
	memset (empty, '\0', buffsize);
      }
    while (true)
      {
	bool eof;		// tell if we are at end of file
	bool cant_wait;		// tell if we need to flush buffers
	/* Check if we have to cancel the copy */
	if (get_current_mode () == CANCEL)
	  return -2;
	/* Read */
	len = (int) read (in_fd, buffer, buffsize);
	if (-1 == len)
	  return -1;
	eof = (len != buffsize);
	if (gap)
	  {
	    assert (0 == buffsize % sizeof (*buffer));
	    /* Is the buffer empty ? */
	    is_empty = 1;
	    // at EOF we will take in account previously read datas
	    // but that is not important
	    for (uint i = 0; i < buffsize / sizeof (*buffer); i++)
	      if (buffer[i])
		{
		  is_empty = 0;	// no
		  break;
		}
	    if (eof)
	      is_empty = 0;	// force write of the last buffer
	    if (is_empty)
	      empty_buffs++;	// (can't overflow, see cant_wait two lines down)
	  }
	/* if in sparse mode, we'll wait for eof or data, or int overflow before writing */
	cant_wait = !is_empty || eof
	  || (empty_buffs + 1) * buffsize > INT_MAX;
	if (gap && cant_wait)
	  {
	    /* Should we make a hole ? */
	    if (empty_buffs >= gap && len != 0)	// Don't finish with a hole if len == 0
	      {
		if (-1 ==
		    lseek (out_fd, (off_t) (empty_buffs * buffsize),
			   SEEK_CUR))
		  return -1;
		empty_buffs = 0;
	      }
	    else
	      {
		// Write empty space
		for (; empty_buffs; empty_buffs--)
		  if (buffsize != write (out_fd, empty, buffsize))
		    return -1;
		assert (0 == empty_buffs);
	      }
	  }
	if (!gap || cant_wait)
	  {
	    if (len != write (out_fd, buffer, (uint) len))
	      return -1;
	  }
	if (eof)
	  break;
      }
  }
  /* Verify we didn't miss anything */
  {
    struct stat in_stats;
    struct stat out_stats;
    if (fstat (in_fd, &in_stats))
      return -1;
    if (fstat (out_fd, &out_stats))
      return -1;
    if (out_stats.st_size != in_stats.st_size)
      {
	errno = 0;		// the error would be in the check and so meaningless
	return -1;
      }
  }
  return 1;
}


/* Mark a file as shaked
 */
static void
capture (struct accused *a, struct law *l)
{
  assert (a && l);
  /* Update position time */
  if (l->xattr && -1 == set_ptime (a->fd))
    {
      error (0, errno, "%s: failed to set position time, check user_xattr",
	     a->name);
    }
  return;
}

/* Restore mtime of a file
 */
static void
release (struct accused *a, struct law *l)
{
  assert (a && l);
  assert (a->fd >= 0);
  /* restore mtime */
  {
    struct timeval tv[2];
    tv[0].tv_sec = a->atime;
    tv[0].tv_usec = 0;
    tv[1].tv_sec = a->mtime;
    tv[1].tv_usec = 0;
    futimes (a->fd, tv);
  }
  return;
}


int
shake_reg (struct accused *a, struct law *l)
{
  assert (a && l);
  assert (S_ISREG (a->mode)), assert (a->guilty);
  const uint GAP = MAGICLEAP * 4;
  char *msg;
  int res;
  if (l->pretend)
    return 0;
  capture (a, l);
  if (-1 ==
      asprintf (&msg,
		"%s: unrecoverable internal error ! file has been saved at %s",
		a->name, l->tmpname))
    {
      int errsv = errno;
      unlink (l->tmpname);	// could work
      error (1, errsv, "%s: failed to initialise failure manager", a->name);
    }

  /* Do the backup */
  res = fcopy (a->fd, l->tmpfd, MAGICLEAP);
  if (-1 == res)
    {
      // Backup failed
      int errsv = errno;
      unlink (l->tmpname);	// could work
      error (1, errsv, "%s: temporary copy failed", a->name);
    }
  else if (-2 == res
	   // Try to disable most signals (except critical ones, see signals.h)
	   || 0 != enter_critical_mode (msg))
    {
      // Received a cancel
      assert (get_current_mode () == CANCEL);
      errno = 0;
      return -1;
    }
  /*  Ask the FS to put the file at a new place, without losing metadatas
   * nor hard links. Works on ReiserFS but should be tested on other filesystems
   */
  if (0 > ftruncate (a->fd, (off_t) 0))
    error (1, errno,
	   "%s: failed to ftruncate() ! file have been saved at %s",
	   a->name, l->tmpname);
  /* Do the reverse copying */
  if (0 > fcopy (l->tmpfd, a->fd, GAP))
    error (1, errno, "%s: restore failed ! file have been saved at %s",
	   a->name, l->tmpname);
  /* Restore most signals */
  release (a, l);
  free (msg);
  return 0;
}


/*  For use by qsort().
 */
static int
atimesort (const void *a, const void *b)
{
  assert (a && b);
  /* Cast *a and *b */
  char *aname = *((char *const *) a);
  char *bname = *((char *const *) b);
  assert (aname && bname);
  /* Get atimes */
  struct stat astat;
  struct stat bstat;
  if (-1 == stat (aname, &astat) || -1 == stat (bname, &bstat))
    {
      /*  If we abort here, it will be a DoS caused by a race condition.
       * So vomit a cryptic error message and go on.
       */
#ifndef NDEBUG
      error (0, errno, "stat() of %s or %s failed", aname, bname);
#endif
      return 0;
    }
  // Using modulo to prevent int overflow
  return (int) ((bstat.st_atime - astat.st_atime) % 2);
}

char **
list_dir (char *name, int sort)
{
  assert (name);
  const size_t BUFFSTEP = 128;
  size_t namel = strlen (name);
  /* Read entries one by one and store their name in a buffer */
  uint n = 0;
  char **buff = malloc (sizeof (*buff) * BUFFSTEP);
  if (!buff)
    {
      error (0, errno, "%s: malloc() failed", name);
      return NULL;
    }
  buff[0] = NULL;
  struct dirent *ent = NULL;
  DIR *dir = opendir (name);
  if (!dir)
    {
      error (0, errno, "%s: opendir() failed", name);
      return NULL;
    }
  for (n = 0; (ent = readdir (dir)); n++)
    {
      char *dname = ent->d_name;
      char *fname;		// full name
      if (n == INT_MAX)
	{
	  error (0, 0, "%s: more than %u files", name, INT_MAX - 1);
	  break;
	}
      /* ignore "." and ".." */
      if (0 == strcmp (dname, ".") || 0 == strcmp (dname, ".."))
	{
	  n--;
	  continue;
	}
      /* Does the buffer need to be extended ? */
      if (!((n + 2) % BUFFSTEP))	// + 1 for buff[n], + 1 for buff[n+1]=NULL
	{
	  char **nbuff = realloc (buff, (n + 2 + BUFFSTEP) * sizeof (*buff));
	  if (!nbuff)
	    {
	      error (0, errno, "%s: realloc() failed", name);
	      break;
	    }
	  buff = nbuff;
	}
      /* get and store the complete path relative to cwd */
      fname = malloc (namel + strlen (dname) + 2);
      if (!fname)
	{
	  error (0, errno, "%s: malloc() failed", name);
	  /* Try with the next file */
	  n--;
	  continue;
	}
      strcpy (fname, name);
      fname[namel] = '/';
      strcpy (fname + namel + 1, dname);
      buff[n] = fname;
    }
  /* They are some breaks in the for loop */
  if (sort)
    qsort (buff, n, sizeof (*buff), &atimesort);
  /* A NULL entry will mark the end of the buffer */
  buff[n] = NULL;
  closedir (dir);
  return buff;
}

char **
list_stdin (void)
{
  const size_t BUFFSTEP = 32;
  uint n = 0;
  size_t len = 0;
  char *line = NULL;
  char **buff = malloc (BUFFSTEP * sizeof (*buff));
  if (!buff)
    error (1, errno, "-: malloc() failed");
  /* for each line */
  for (; -1 != getline (&line, &len, stdin); line = NULL, len = 0)
    {
      if (n == INT_MAX)
	{
	  error (0, 0, "-: more than %i files", INT_MAX - 1);
	  return NULL;
	}
      /* remove the end of line */
      *strchrnul (line, '\n') = '\0';
      /* Does the buffer need to be extended ? */
      if (!((n + 2) % BUFFSTEP))	// + 1 for buff[n], + 1 for buff[n+1]=NULL
	{
	  char **nbuff = realloc (buff, (n + 2 + BUFFSTEP) * sizeof (*buff));
	  if (!nbuff)
	    {
	      error (0, errno, "s: realloc() failed");
	      close_list (buff);
	      free (line);
	      return NULL;
	    }
	  buff = nbuff;
	}
      /* Ignore "" */
      if ('\0' == *line)
	{
	  free (line);
	  continue;
	}
      /* Add the line into the buffer */
      buff[n] = line;
      n++;
    }
  free (line);
  /* A NULL entry will mark the end of the buffer */
  buff[n] = NULL;
  if (n)
    qsort (buff, n - 1, sizeof (*buff), &atimesort);
  return buff;
}

void
close_list (char **flist)
{
  assert (flist);
  for (char **oflist = flist; *oflist; oflist++)
    free (*oflist);
  free (flist);
}
