/* -*- mode: linux-c -*-
 *
 *      Copyright (C) 2005-2006 Cyrill Gorcunov <gclion@mail.ru>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *      May God bless you!
 *
 */

#ifndef XONCLOCK_H_
#define XONCLOCK_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define msg(fmt, arg ...)						\
do {									\
	printf(PACKAGE_NAME "-" PACKAGE_VERSION ": %s:%i " fmt "\n",	\
	       __FILE__, __LINE__, ## arg);	\
} while(0)

/* for BSD-systems compability */
#ifndef SA_ONESHOT
#define SA_ONESHOT SA_RESETHAND
#endif
#ifndef SA_NOMASK
#define SA_NOMASK SA_NODEFER
#endif

/* for debug print */
#define DPRINT(msg)	fprintf(stdout, msg)

#define RCFILE		".xonclockrc"
#define RCFILE_LEN	15	/* RCFILE_LEN must be >= len(RCFILE) + 3 */
#define PATH_LEN	256
#define DBLCLICKTIME	500	/* how many milliseconds betw
				 * two mouse clicks */

#define MIN_CLOCK_WIDTH		20
#define MAX_CLOCK_WIDTH		3000
#define MIN_CLOCK_HEIGHT	20
#define MAX_CLOCK_HEIGHT	3000

int parse_rgba_string(char *stream,
		      unsigned char *red,
		      unsigned char *green,
		      unsigned char *blue,
		      unsigned char *alpha);
void sig_time(int signum);
void sig_term(int signum);
void sig_int(int signum);
void xonclock_clear(void);
void print_version(void);

#endif/*XONCLOCK_H_*/
