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

#include <stdio.h>		// puts(), printf(), putchar()

#include "config.h"
#include "msg.h"

void
show_help (void)
{
  puts ("\
Usage: shake [OPTION]... [FILE|DIRECTORY]...\n\
Rewrite fragmented or misplaced files, maybe improving performance.\n\
Reads file list from standard input if there is no files in the arguments.\n\
You have to mount your partition with the user_xattr option.\n\
\n\
  -c, --max-crumbc	max number of crumbs\n\
  -C, --max-fragc	max number of fragments\n\
  -d, --max-deviance	max distance between file start and it's ideal position\n\
  -h, --help		you're looking at me !\n\
  -L, --no-locks	don't put a lock on written files\n\
  -m, --many-fs		shake subdirectories that are on different filesystems\n\
  -n, --new		age of \"new\" files, which will be shak()ed\n\
  -o, --old		age of \"old\" files, which won't be shak()ed\n\
  -p, --pretend		don't alter files\n\
  -r, --crumbratio	ratio of the file under which a fragment is a crumb\n\
  -s, --smallsize	the size under which a file is considered small\n\
  -S, --bigsize		the size under which a file is considered big\n\
  -t, --small-tolerance	multiply crumbratio and divide maxfnumber of small files\n\
  -T, --big-tolerance	multiply crumbratio and divide maxfnumber of big files\n\
  -v, --verbose		increase the verbosity level\n\
  -V, --version		show version number and copyright\n\
  -X, --no-xattr	disable usage of xattr\n\
Report bugs at https://github.com/unbrice/shake/issues\
");
}

void
show_version (void)
{
  puts ("\
Shake " VERSION "\n\
Copyright (C) 2006-2011 Brice Arnould.\n\
Shake comes with ABSOLUTELY NO WARRANTY. You may redistribute copies of Shake\n\
under the terms of the GNU General Public License version or greater.\n\
For more information about these matters, see the file named GPL.txt.\
");
}

void
show_header (struct law *l)
{
  if (l->verbosity)
    printf ("IDEAL\tSTART\tEND\tFRAGC\tCRUMBC\tAGE\tSHOCKED\tNAME");
  if (l->verbosity >= 3)
    puts ("\tFRAGS");
  else if (l->verbosity)
    putchar ('\n');
}

void
show_reg (struct accused *a, struct law *l)
{
  /* Show file status */
  printf ("%lli\t%lli\t%lli\t%i\t%i\t%i\t%i\t%s",
	  a->ideal, a->start / 1024, a->end / 1024, a->fragc, a->crumbc,
	  (int) (a->age / 3600 / 24), a->guilty, a->name);
  /* And, eventualy, list of frags and crumbs */
  if (l->verbosity > 2 && a->poslog && a->poslog[0] != -1)
    {
      uint n;
      putchar ('\t');
      for (n = 0; a->sizelog[n + 1] != -1; n++)
	printf ("%lli:%lli,", a->poslog[n] / 1024, a->sizelog[n] / 1024);
      printf ("%lli:%lli\n", a->poslog[n] / 1024, a->sizelog[n] / 1024);
    }
  else
    putchar ('\n');
}
