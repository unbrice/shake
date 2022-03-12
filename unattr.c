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

/*** unattr: quick and dirty hack to remove unwanted Xattrs ****/

#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <error.h>
#include <string.h>
#include <attr/attributes.h>	// flistxattr
#include <sys/types.h>		// open()
#include <sys/stat.h>		// open()
#include <sys/xattr.h>  // fremovexattr()
#include <fcntl.h>		// open()
#include <unistd.h>		// close()
#include "linux.h"		// getopt()

#include "config.h"
#include "executive.h"
void strip (char *name, char **attrs);

void
show_help (void)
{
  puts ("\
Usage: unattr [OPTION]... <FILE|DIRECTORY>...\n\
Remove choosen xattr from files, recursively.\n\
\n\
  -a, --attr		specify the name of a new attribute\n\
  -h, --help		help\n\
  -V, --version		show version number and copyright\n\
Report bugs at https://github.com/unbrice/shake/issues\
");
}

/* If name refer to a regular file, call strip() on it.
 * If name refer to a directory, call itself on dir entries.
 * Else, or if open failed, do nothing.
 */
void
look (char *name, char **attr)
{
  assert (name && attr);
  struct stat st;
  /* stat */
  if (-1 == lstat (name, &st))
    error (0, errno, "%s: stat() failed", name);
  else if (S_ISREG (st.st_mode))	// regular file
    strip (name, attr);
  else if (S_ISDIR (st.st_mode))	// directory
    {
      /* list it */
      char **flist = list_dir (name, false);
      if (!flist)
	{
	  error (0, 0, "%s: list_dir() failed", name);
	  return;
	}
      /* Go through the list */
      for (char **name = flist; *name; name++)
	{
	  look (*name, attr);
	  free (*name);
	}
      free (flist);
    }
}

void
show_version (void)
{
  puts ("\
Unattr " VERSION "\n\
Copyright (C) 2006-2008 Brice Arnould.\n\
Unattr comes with ABSOLUTELY NO WARRANTY. You may redistribute copies of Unattr\n\
under the terms of the GNU General Public License. For more information about\n\
these matters, see the file named GPL.txt.\
");
}

/* This function remove attributes attr[0], attr[1]... attr[n]
 * with attr[n] == NULL, from the file named name
 */
void
strip (char *name, char **attr)
{
  assert (name), assert (attr);
  int fd = open (name, O_WRONLY);
  if (0 > fd)
    {
      error (0, errno, "%s: open() failed", name);
      return;
    }
  for (char **attr = attr; *attr; attr++)
    fremovexattr (fd, *attr);
  close (fd);
}

int
main (int argc, char **argv)
{
  char **attr = NULL;
  /* Parse opts */
  {
    const uint BUFSTEP = 32;
    uint pos = 0;
    while (1)
      {
	int c;
	static const struct option long_options[] = {
	  {"attr", required_argument, NULL, 'a'},
	  {"help", no_argument, NULL, 'h'},
	  {"version", no_argument, NULL, 'V'},
	  {0, 0, 0, 0}
	};
	c = getopt_long (argc, argv, "a:hV", long_options, NULL);
	if (c == -1)
	  break;
	switch (c)
	  {
	  case 'a':
	    if (0 == pos % BUFSTEP)
	      attr = realloc (attr, sizeof (*attr) * (pos + BUFSTEP + 1));
	    if (!attr)
	      error (1, errno, "alloc() failed");
	    attr[pos] = strdup (optarg);
	    if (!attr[pos++])
	      error (1, errno, "alloc() failed");
	    break;
	  case 'h':
	    show_help ();
	    return 0;
	  case 'V':
	    show_version ();
	    return 0;
	  case 0:
	  case '?':
	  default:
	    error (1, 0, "invalid args, aborting");
	  }
      }
    if (!attr)
      return 0;
    attr[pos] = NULL;
  }
  /* For each name */
  if (attr)
    for (; optind < argc; optind++)
      look (argv[optind], attr);
  /* free */
  close_list (attr);
  return 0;
}
