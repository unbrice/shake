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
#ifndef LINUX_H
# define LINUX_H
#define _GNU_SOURCE
#include <getopt.h>		// getopt_long()
#include <alloca.h>		// alloca()
#include <stdio.h>		// getline
#include <sys/time.h>		// struct timeval
#include "judge.h"

int futimes (int fd, const struct timeval tv[2]);
int set_ptime (int fd);
time_t get_ptime (int fd);
int get_testimony (struct accused *a, struct law *l);
#endif
