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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <pwd.h>

#include "parse.h"
#include "xonclock.h"
#include "placement.h"

static unsigned char path_buf[PATH_BUF_SZ];

static char *buf = NULL;

char * trim(char *stream);
char * trim_comment(char *stream);
int get_opt_val(char *stream, char **opt, char **val);

name_ival_t BOOL_vals[] = {
	{"true", 1},
	{"false", 0}
};

name_ival_t POSITION_vals[] = {
	{"TOP-LEFT", TOP_LEFT},
	{"TOP-CENTER", TOP_CENTER},
	{"TOP-RIGHT", TOP_RIGHT},
	{"MIDDLE-LEFT", MIDDLE_LEFT},
	{"MIDDLE-CENTER", MIDDLE_CENTER},
	{"MIDDLE-RIGHT", MIDDLE_RIGHT},
	{"BOTTOM-LEFT", BOTTOM_LEFT},
	{"BOTTOM-CENTER", BOTTOM_CENTER},
	{"BOTTOM-RIGHT", BOTTOM_RIGHT}
};

/* available options in .xonclockrc file */
conf_opt_t conf_opts[] = {
	{"skin", NULL, -1},
	{"position", NULL, -1},
	{"offset-v", NULL, -1},
	{"offset-h", NULL, -1},
	{"hour-length", NULL, -1},
	{"hour-width", NULL, -1},
	{"minute-length", NULL, -1},
	{"minute-width", NULL, -1},
	{"second-length", NULL, -1},
	{"second-width", NULL, -1},
	{"color-hour", NULL, -1},
	{"color-minute", NULL, -1},
	{"color-second", NULL, -1},
	{"show-seconds", NULL, -1},
	{"sleep", NULL, -1},
	{"show-date", NULL, -1},
	{"date-font", NULL, -1},
	{"date-color", NULL, -1},
	{"date-format", NULL, -1},
	{"date-position", NULL, -1},
	{"date-offset-v", NULL, -1},
	{"date-offset-h", NULL, -1},
	{"date-clip-format", NULL, -1},
	{"no-winredirect", NULL, -1},
	{"hands-rotation-axis", NULL, -1},
	{"top", NULL, -1},
	{"overlap", NULL, -1},
	{"movable", NULL, -1},
	{"use-background", NULL, -1},
	{"remerge-sleep", NULL, -1},
	{"tz-hour", NULL, -1},
	{"tz-min", NULL, -1},
	{"alpha-threshold", NULL, -1},
	{NULL, NULL, -1} /* terminator */
};


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	parse_config
 * $Desc:	parses config file pointed by 'path'
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int parse_config(char *path)
{
	FILE *file = NULL;
	long size;
	size_t readed;
	char *line, *opt, *val;
	int i;

	/* check entry */
	if (!path) {
		msg("NULL pointer passed");
		return -1;
	}

	file = fopen(path, "r");
	if (!file) {
		msg("Unable to open config file: %s", path);
		return -1;
	}

	fseek(file, 0, SEEK_END);
	size = ftell(file);
	rewind(file);

	if (size > 0) {
		free_opt_buf();
		buf = calloc(1, size + 1); /* it must be zero-ended string */
		if (!buf) {
			fclose(file);
			msg("No free memory");
			return -1;
		}
	} else {
		fclose(file);
		msg("Config file too small: %s", path);
		return -1;
	}

	/* read a whole file */
	readed = fread(buf, size, 1, file);
	if (readed != 1) {
		fclose(file);
		free_opt_buf();
		msg("Unable to read config file: %s", path);
		return -1;
	}

	/* OK, now we need to parse it */
	line = strtok(buf, "\r\n");
	while(line) {
		line = trim_comment(line);
		if (get_opt_val(line, &opt, &val) != -1) {
			for (i = 0; i < GET_COUNT(conf_opts,conf_opt_t) - 1; i++) {
				if (strcasecmp(opt, conf_opts[i].opt_name) == 0) {
					conf_opts[i].opt_val = val;
					break;
				}
			}
		}
		line = strtok(NULL, "\r\n");
	}

	fclose(file);

	return 0;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	free_opt_buf
 * $Desc:	free option's buffer
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void
free_opt_buf(void)
{
	if (buf) {
		free(buf);
		buf = NULL;
	}
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	get_conf_opt
 * $Desc:	returns value associated with the 'opt' option
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
char * get_conf_opt(char *opt)
{
	char *opt_name = NULL;
	char *opt_val = NULL;
	int i = -1;

	/* yes, it's not as fast as hashing do but
	 * we have smalish count of parameters */
	opt_name = conf_opts[i].opt_name;
	while((opt_name = conf_opts[++i].opt_name) != NULL) {
		if (strcasecmp(opt_name, opt) == 0) {
			opt_val = conf_opts[i].opt_val;
			break;
		}
	}

	return opt_val;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	trim
 * $Desc:	removes leading and trailing spaces from a stream
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
char * trim(char *stream)
{
	char *p = stream;
	char *pp = NULL;
	int len;

	if (stream) {
		len = strlen(stream);
		while(*p != 0x0) { /* left part */
			if (*p == '\t' || *p == ' ') {
				p++, len--;
			} else {
				break;
			}
		}
		if (len > 0) {	/* right part */
			pp = p + len - 1;
			while(pp > p) {
				if (*pp == '\t' || *pp == ' ') {
					pp--, len--;
				} else {
					*(pp + 1) = 0x0;
					break;
				}
			}
		} else {
			p = NULL;
		}
	}
	return p;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	trim_comment
 * $Desc:	just trim comment from a stream
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
char * trim_comment(char *stream)
{
	char *p = NULL;

	if (stream) {
		p = stream;
		while(*p != 0x0) {
			if (*p == '#') {
				*p = 0x0;
				break;
			}
			p++;
		}
	}

	return stream;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	get_opt_val
 * $Desc:	just split 'a=b' to pair 'a' and 'b'
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int get_opt_val(char *stream,
		char **opt,
		char **val)
{
	int rc = -1;
	char *p = stream;

	if (p) {
		while(*p != 0x0) {
			if (*p == '=') {
				*p = 0x0;
				*opt = trim(stream);
				*val = trim((p + 1));
				if (*opt && *val)
					rc = 1;
				break;
			}
			p++;
		}
	}

	return rc;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	get_opt_BOOL
 * $Desc:	parses BOOL-formatted sring
 * $Ret:	0 on OK and set val or -1 on error
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int get_opt_BOOL(char *opt_val, int *ret_val)
{
	int i;
	int rc = -1;

	if (!opt_val || !ret_val)
		return rc;

	for (i = 0; i < GET_COUNT(BOOL_vals, name_ival_t); i++) {
		if (strcasecmp(BOOL_vals[i].name, opt_val) == 0) {
			*ret_val = BOOL_vals[i].val;
			rc = 0;
			break;
		}
	}

	return rc;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	get_opt_POSITION
 * $Desc:	parses POSITION-formatted sring
 * $Ret:	0 on OK and set val or -1 on error
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int get_opt_POSITION(char *opt_val, int *ret_val)
{
	int i;
	int rc = -1;

	if (!opt_val || !ret_val)
		return rc;

	for (i = 0; i < GET_COUNT(POSITION_vals, name_ival_t); i++) {
		if (strcasecmp(POSITION_vals[i].name, opt_val) == 0) {
			*ret_val = POSITION_vals[i].val;
			rc = 0;
			break;
		}
	}

	return rc;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	get_opt_STRING
 * $Desc:	parses STRING-formatted sring
 * $Ret:	0 on OK and set val or -1 on error
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int get_opt_STRING(char *opt_val, char **ret_val)
{
	int rc = -1;

	if (!opt_val || !ret_val)
		return rc;

	*ret_val = opt_val;
	rc = 0;

	return rc;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	get_opt_COORDS
 * $Desc:	parses COORDS-formatted sring
 * $Ret:	0 on OK and set x,y or -1 on error
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int get_opt_COORDS(char *opt_val, int *ret_x, int *ret_y)
{
	int rc = -1;
	char *sx, *sy;

	if (!opt_val || !ret_x || !ret_y)
		return rc;

	sx = strtok(opt_val, "/");
	sy = strtok(NULL, "/");
	if (sx && sy) {
		*ret_x = atoi(sx);
		*ret_y = atoi(sy);
		rc = 0;
	}

	return rc;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	get_opt_COLORA
 * $Desc:	parses COLORA-formatted sring
 * $Ret:	0 on OK and set r,g,b,a or -1 on error
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int get_opt_COLORA(char *opt_val,
		   unsigned char *ret_r,
		   unsigned char *ret_g,
		   unsigned char *ret_b,
		   unsigned char *ret_a)
{
	int rc = -1;
	char *sr, *sg, *sb, *sa;

	if (!opt_val || !ret_r || !ret_g || !ret_b || !ret_a)
		return rc;

	sr = strtok(opt_val, "/");
	sg = strtok(NULL, "/");
	sb = strtok(NULL, "/");
	sa = strtok(NULL, "/");

	if (sr && sg && sb && sa) {
		*ret_r = (unsigned char)atoi(sr);
		*ret_g = (unsigned char)atoi(sg);
		*ret_b = (unsigned char)atoi(sb);
		*ret_a = (unsigned char)atoi(sa);
		rc = 0;
	}

	return rc;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	get_opt_NUMBER
 * $Desc:	parses NUMBER-formatted sring
 * $Ret:	0 on OK and set val or -1 on error
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int get_opt_NUMBER(char *opt_val, int *ret_val)
{
	int rc = -1;

	if (!opt_val || !ret_val)
		return rc;

	*ret_val = atoi(opt_val);
	rc = 0;

	return rc;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	get_opt_NUMBER_LONG
 * $Desc:	parses NUMBER-formatted sring
 * $Ret:	0 on OK and set val or -1 on error
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int get_opt_NUMBER_LONG(char *opt_val, long *ret_val)
{
	int rc = -1;

	if (!opt_val || !ret_val)
		return rc;

	*ret_val = atol(opt_val);
	rc = 0;

	return rc;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	get_opt_ABSNUMBER
 * $Desc:	parses ABSNUMBER-formatted sring
 * $Ret:	0 on OK and set val or -1 on error
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int get_opt_ABSNUMBER(char *opt_val, int *ret_val)
{
	int rc = -1;

	if (!opt_val || !ret_val)
		return rc;

	*ret_val = (int) abs(atoi(opt_val));
	rc = 0;

	return rc;
}


/* just resolve the 'path'
 * now '~/' at the start of the path is supported
 * returns pointer to local buffer on success or NULL on error */
unsigned char * get_resolved_path(unsigned char *path)
{
	unsigned int len = 0;
	unsigned int home_len = 0;
	struct passwd *pass = NULL;

	if (!path || *path == 0x0) {
		msg("NULL pointer passed");
		return NULL;
	}

	len = strlen(path);

	if (path[0] != '~' || path[1] != '/') {
		if (len > PATH_BUF_SZ - 1) {
			msg("Too long path");
			return NULL;
		}
		memset(path_buf, 0x0, PATH_BUF_SZ);
		memcpy(path_buf, path, len + 1);
	} else {
		pass = getpwuid(getuid());
		if (!pass || !pass->pw_dir || pass->pw_dir[0] == 0) {
			msg("Unable to retrieve user data");
			return NULL;
		}
		len -= 2;
		path += 2;	/* skip '~/' */
		home_len = strlen(pass->pw_dir);
		/* we may need to insert additional slashes and
		 * zero at end of string so that is why we use -3 */
		if ((home_len + len) > PATH_BUF_SZ - 3) {
			msg("Too long path");
			return NULL;
		}
		memset(path_buf, 0x0, PATH_BUF_SZ);
		memcpy(path_buf, pass->pw_dir, home_len);
		if (path_buf[home_len] != '/') {
			path_buf[home_len] = '/';
		}
		strcat(path_buf, path);
	}

	return path_buf;
}
