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

/*** SHAKE: rewrite fragmented files, maybe improving performance ****/

/***************************************************************
 * As you'll see, I don't understand how ReiserFS, nor extN or
 * others works. I hacked this to waste my time and for that
 * it was just fine. So... please don't be too rude.
 *
 *  This software could be used to reduce fragmentation,
 * without using dangerous, low level and filesystem specific
 * tools, and while the system is used (altough it is yet more
 * inefficient in this case).
 *  It's main idea is that the filesystem know where to store
 * files, altough a choice that were good month ago could be
 * bad now.
 *  So it looks for fragmented files and read/write them again
 * ("shake them") to help the filesystem to do its job.
 *  Regarding this, ReiserFS and Ext4 (the only FS I tested) have
 * been impressive : they stored closely files of the same directory
 * and blocks of fragmented files...
 *  Shake also try to shake files which have a very old ctime, in
 * an attempt to defragment the free space.
 *  Files of the same directory are considered as "friends", so those
 * who have similar atime but are far from each other will also
 * be shaked (should be usefull mainly for apt or portage cache,
 * which have a lot of little files).
 *  Those features implies that shake don't alter atime, but do
 * so for ctime in order to be more efficient in incremental use.
 *  You can see yourself where your filesystem put data with
 * -pv. A 0 means that the file have no reserved block, maybe
 * beacause it's data are within the file descriptor, or share a
 * block with others.
 *  Please note that after a run of shake, a file can be actually
 * more fragmented, if the file have been made sparse. But the
 * storage should be more efficient.
 *  It would be great for me to have information about the behaviour
 * of shake on other filesystems.
 *
 *  This program is meant to be run on GNU/Linux only. However
 * if someone points me to an equivalent of FIBMAP under his
 * free and posix-compliant system, I'll try to port it (would need
 * to remove GNU extensions or to use GNUlib...).
 *
 ***************************************************************/

#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <error.h>
#include <string.h>

#include <unistd.h>		// unlink()
#include <sys/types.h>		// umask()
#include <sys/stat.h>		// umask()
#include "linux.h"
#include "judge.h"
#include "executive.h"
#include "msg.h"
#include "signals.h"




/*  Read the string at str. If it is not a number, or if the number is
 * < min, exit with an error. Else return the number as a unsigned
 * integer.
 */
static uint
argtoi (char *str, int min, char *name)
{
  char *endptr;
  uint res;
  assert (str && name);
  res = (uint) strtol (str, &endptr, 10);
  if (str == endptr || res < min)
    error (1, 0, "%s must be >= %i", name, min);
  return res;
}

/*  Read the string at str. If it is not a number, or if the number is
 * < min, exit with an error. Else return the number as a float.
 */
static float
argtof (char *str, int min, char *name)
{
  char *endptr;
  float res;
  assert (str && name);
  res = strtof (str, &endptr);
  if (str == endptr || res < min)
    error (1, 0, "%s must be >= %i", name, min);
  return res;
}

/*  This function takes argc, argv and a law.
 *  It adapt the law to options specified by user, reorder argv to
 * put file names at the end, and return an integer corresponding
 * to the position of the first file name (like getopt() do).
 */
static int
parseopts (int argc, char **restrict argv, struct law *restrict l)
{
  const time_t day = 24 * 60 * 60;
  const time_t kB = 1000;
  const time_t mB = 1000 * kB;
  /* Default values - fields are described in judge.h */
  {
    l->maxfragc = 21;		// 10 sec per frag on a 210 sec OGG Vorbis
    l->crumbratio = 0.95 / 100;	// 2 sec of a 210 sec OGG Vorbis
    l->maxcrumbc = 9;		// A magic number.
    l->smallsize = 16 * kB;	// Config files
    l->smallsize_tol = 0.1;	// 2 frag max for a config file
    l->bigsize = 95 * mB;	// Takes 7 sec to shake()
    l->bigsize_tol = MAX_TOL;
    l->maxdeviance = MAGICLEAP * 4;
    l->old = 8 * 31 * day;
    l->new = 1 * 31 * day;
    l->pretend = false;
    l->verbosity = 0;
    l->locks = true;
    l->kingdom = 0;		// --many-fs disabled
    l->xattr = 1;
  }
  /* Like the manpage said .. */
  while (1)
    {
      int c;
      /* Associate long names to short ones */
      static const struct option long_options[] = {
	{"max-crumbc", required_argument, NULL, 'c'},
	{"max-fragc", required_argument, NULL, 'C'},
	{"max-deviance", required_argument, NULL, 'd'},
	{"help", no_argument, NULL, 'h'},
	{"no-locks", no_argument, NULL, 'L'},
	{"many-fs", no_argument, NULL, 'm'},
	{"new", required_argument, NULL, 'n'},
	{"old", required_argument, NULL, 'o'},
	{"pretend", no_argument, NULL, 'p'},
	{"verbose", no_argument, NULL, 'v'},
	{"crumbratio", required_argument, NULL, 'r'},
	{"smallsize", required_argument, NULL, 's'},
	{"bigsize", required_argument, NULL, 'S'},
	{"small-tolerance", required_argument, NULL, 't'},
	{"big-tolerance", required_argument, NULL, 'T'},
	{"version", no_argument, NULL, 'V'},
	{"no-xattr", no_argument, NULL, 'X'},
	{0, 0, 0, 0}
      };
      c =
	getopt_long (argc, argv, "c:C:d:hL:mn:o:pvr:s:S:t:T:VWX",
		     long_options, NULL);
      if (c == -1)
	break;
      switch (c)
	{
	case 'c':
	  l->maxcrumbc = argtoi (optarg, 0, "max-crumbc");
	  break;
	case 'C':
	  l->maxfragc = argtoi (optarg, 0, "max-fragc");
	  break;
	case 'd':
	  l->maxdeviance = argtoi (optarg, 0, "max-deviance");
	  break;
	case 'h':
	  show_help ();
	  exit (0);
	case 'L':
	  l->locks = false;
	  break;
	case 'm':
	  l->kingdom = (dev_t) - 1;	// ignore filesystems
	  break;
	case 'n':
	  l->new = day * argtoi (optarg, 0, "new");
	  if (l->new > l->old)
	    l->old = l->new;
	  break;
	case 'o':
	  l->old = day * argtoi (optarg, 0, "old");
	  if (l->old < l->new)
	    l->new = l->old;
	  break;
	case 'p':
	  l->pretend = true;
	  break;
	case 'r':
	  l->crumbratio = argtof (optarg, 0, "crumbratio");
	  break;
	case 's':
	  l->smallsize = kB * argtoi (optarg, 0, "small-size");
	  if (l->smallsize > l->bigsize)
	    l->bigsize = l->smallsize;
	  break;
	case 'S':
	  l->bigsize = kB * argtoi (optarg, 0, "big-size");
	  if (l->bigsize < l->smallsize)
	    l->smallsize = l->bigsize;
	  break;
	case 't':
	  l->smallsize_tol = argtof (optarg, 0, "tolerance");
	  break;
	case 'T':
	  l->bigsize_tol = argtof (optarg, 0, "tolerance");
	  break;
	case 'v':
	  l->verbosity++;
	  break;
	case 'V':
	  show_version ();
	  exit (0);
	case 'X':
	  l->xattr = 0;
	  break;
	case 0:
	case '?':
	default:
	  error (1, 0, "invalid args, aborting");
	}
    }
  return optind;
}

/*  Avoid some mkstemp problems with old libc.
 */
static int
xmkstemp (char *basename)
{
  assert (basename);
  mode_t oldmask;
  int fd;
  int errsv;
  if (!basename)
    return -1;
  oldmask = umask (7777);
  fd = mkstemp (basename);
  errsv = errno;
  umask (oldmask);
  errno = errsv;		// restore mkstemp's errno
  return fd;
}

int
main (int argc, char **argv)
{
  char *tmpname;
  int tmpfd;
  struct accused *a;
  struct law l;
  int optind;

  /* Read the law */
  optind = parseopts (argc, argv, &l);
  assert (optind >= 0);

  /* Get a temporary file  */
  {
    tmpname = strdup ("shakeXXXXXX");
    if (!tmpname)
      error (1, errno, "strdup failed");
    tmpfd = xmkstemp (tmpname);
    l.tmpfd = tmpfd;
    l.tmpname = tmpname;
  }
  install_sighandler (tmpname);
  os_specific_setup (tmpname);

  /* Do the stuff (tm) */
  show_header (&l);
  if (optind == argc)
    judge_stdin (NULL, &l);
  else
    for (int i = optind; i != argc; i++)
      {
	a = investigate (argv[i], &l);
	if (NULL == a)
	  continue;		// error have been displayed by investigate()
	if ((dev_t) - 1 != l.kingdom)	// --one-file-system
	  l.kingdom = a->fs;
	judge (a, &l);
	close_case (a, &l);
      }
  unlink (tmpname);
  free (tmpname);
  return 0;
}
