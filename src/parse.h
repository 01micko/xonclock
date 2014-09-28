/* -*- mode: linux-c -*-
 *
 *      Copyright (C) 2006 Cyrill Gorcunov <gclion@mail.ru>
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

#ifndef PARSE_H_
#define PARSE_H_

#define GET_COUNT(name,type) sizeof(name)/sizeof(type)

#define PATH_BUF_SZ		256

enum opt_types {		/* reserved */
	BOOLEAN = 1,		/* true or false */
	STRING = 2,		/* any zero ended string */
	NUMBER = 3,		/* any integer number */
	ABSNUMBER = 4,		/* absolute number */
	COLORA = 5,		/* R/G/B/A */
	COORDS = 6,		/* X/Y */
	POSITION = 7,		/* see enum placement in placement.h */
	LONG_NUM = 8		/* long number */
};

typedef struct conf_opt_s {
	char *opt_name;		/* option name */
	char *opt_val;		/* option value */
	int line_num;		/* reserved */
} conf_opt_t;

typedef struct name_ival_s {
	char *name;		/* name */
	int val;		/* value */
} name_ival_t;

int parse_config(char *path);
char * get_conf_opt(char *opt);
void free_opt_buf(void);

int get_opt_BOOL(char *opt_val, int *ret_val);
int get_opt_POSITION(char *opt_val, int *ret_val);
int get_opt_STRING(char *opt_val, char **ret_val);
int get_opt_COORDS(char *opt_val, int *ret_x, int *ret_y);
int get_opt_COLORA(char *opt_val,
		   unsigned char *ret_r,
		   unsigned char *ret_g,
		   unsigned char *ret_b,
		   unsigned char *ret_a);
int get_opt_NUMBER(char *opt_val, int *ret_val);
int get_opt_NUMBER_LONG(char *opt_val, long *ret_val);
int get_opt_ABSNUMBER(char *opt_val, int *ret_val);
unsigned char * get_resolved_path(unsigned char *path);

#endif/*PARSE_H_*/
