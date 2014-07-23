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

#ifndef JUDGE_H
# define JUDGE_H
#include <stdbool.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>

typedef unsigned int uint;
typedef long long int llint;

/* The distance at wich two adjacent block are considered as two
 *fragment (in byte). ~=(readahead_of_most_disks*2)
 */
//const uint MAGICLEAP = 2* 32 * 1024;
#define MAGICLEAP (2* 32 * 1024)

/*  The time between wich two file are considered as used together
 * (in seconds)
 */
#define MAGICTIME ( 8 )

/*  File with this tolerance won't be defrag
 */
#define MAX_TOL ( -1.0 )

struct law
{
  uint maxfragc;		// max number of fragments
  double crumbratio;		// ratio of the file, a smaller fragment is a crumb
  uint maxcrumbc;		// Allowed number of fragment smaller than crumbratio
  off_t smallsize;		// the size under which a file is considered small
  off_t bigsize;		// the size from which a file is considered as big
  double smallsize_tol;		// multiply crumbratio and divide maxfnumber of small files
  double bigsize_tol;		// " " " " "  for big files
  uint maxdeviance;		// max distance between file start and it's ideal pos
  time_t old;			// Age of "old" files, will be shak()ed
  time_t new;			// Age of "new" files, won't be shak()ed
  bool pretend;			// simulate
  int verbosity;		// set the verbose mode
  bool locks;			// put a lock on written files
  dev_t kingdom;		// file system to examine, ignored if (-1)
  bool xattr;			// use user_xattr
  int tmpfd;
  char *tmpname;
};

/* The file or directory accused of being fragmented
 * Fields are here in the order they are set.
 */
struct accused
{
  mode_t mode;
  char *name;
  int fd;
  off_t size;
  long blocks;			// Number of blocks
  uint fragc;			// Number of fragments
  uint crumbc;			// Number of fragments smaller than crumbratio
  llint start;			// The position of the first block
  llint end;			// The position of the first block
  llint ideal;			// Where the file would idealy start
  time_t atime;			// atime, as returned by stat
  time_t mtime;			// ctime, as returned by stat
  time_t age;			// Min of (atime,ctime,mtime)
  llint *poslog;		// Tab of fragments positions
  llint *sizelog;		// Tab of fragments sizes
  dev_t fs;
  bool guilty;
};

/*  This function return a struct wich describe properties
 * of the file who's name is given.
 */
struct accused *investigate (char *name, struct law *l);

/*  This function free structs allocated by
 * investigate().
 */
void close_case (struct accused *a, struct law *l);



/*  This function call judge on stdin content
 */
int judge_stdin (struct accused *a, struct law *l);

/* Return true if the file is fragmented, else false.
 */
int judge (struct accused *a, struct law *l);

#endif /* JUDGE_H */
