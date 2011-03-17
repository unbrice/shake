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

#ifndef FCOPY_H
# define FCOPY_H
# include "judge.h"

/*  Copy the content of file referenced by in_fd to out_fd
 *  Make file sparse if there's more than gap consecutive '\0',
 * and if gap != 0
 *  Return -1 and set errno if failed, -2 if canceled, anything else
 *  if succeded
 *  This part is crucial as it is the one which do the job and
 * which can break file, it have been carfully read and tested
 * so it would be dangerous to rewrite it... however it's
 * big and ugly -_-.
 */
int fcopy (int in_fd, int out_fd, size_t gap, bool stop_if_input_unlocked);

/*  Make a backup of a file, truncate original to 0, then copy
 * the backup over it.
 * This can be called only when the file is *read* locked. It will
 * take a write lock while operating.
 * This can be called only when in NORMAL mode. It internally set the
 * CRITICAL mode but goes back in NORMAL mode before returning.
 */
int shake_reg (struct accused *a, struct law *l);


/* Return an array containing file names in the named directory,
 * excepted "." and "..".
 * Return NULL in case of error.
 * If sort is true, file names are sorted by atime.
 * LIMIT : INT_MAX files
 */
char **list_dir (char *name, int sort);

/* Return an array containing file names given in stdin
 * excepted "." and "..".
 * File names are sorted by atime.
 * LIMIT : INT_MAX files
 */
char **list_stdin (void);

/* Free arrays allocated by list_dir()
 */
void close_list (char **flist);
#endif
