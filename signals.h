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

#ifndef SIGNALS_H
# define SIGNALS_H


enum mode
{
  /* In this mode, signals are handled with the default handler,  appart
   * that the temporary file will be deleted.
   * You can enter this mode from any other.
   */
  NORMAL = 42,
  /* In this mode :
   *  signals raised by internal errors, such as SIGSEV, stop the program
   * and show informations about the error (char *msg)
   *  signals that suspend the activity works as usual
   *  others are suspended
   * It is intended to be used when current_tempfile is the only copy
   * of a file.
   * You can enter in this mode from NORMAL.
   */
  CRITICAL,
};

/*  Set signals.c/handle_signals() as the default handler, tempfile
 * as the current temporary file, then call enter_normal_mode
 */
void install_sighandler (const char *tempfile);

/* Enter CRITICAL mode (see above), msg is the message to display in
 * case of failure.
 */
void enter_critical_mode (const char *msg);

/* Enter NORMAL mode (see above).
 */
void enter_normal_mode (void);

#endif
