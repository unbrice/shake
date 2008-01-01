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
#ifndef FCOPY_H
# define FCOPY_H
# include "judge.h"
#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))
int fcopy (int in_fd, int out_fd, size_t gap);
char **list_dir (char *name, int sort);
char **list_stdin (void);
int shake_reg (struct accused *a, struct law *l);
void close_list (char **flist);
#endif
