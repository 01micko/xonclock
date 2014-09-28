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
#ifndef CACHE_H_
#define CACHE_H_

#include <stdlib.h>
#include <X11/Xlib.h>

#include "placement.h"

#define xo_bzero(mem, size)	memset((void *)mem, 0x0, (size_t)(size))
#define tz_update(tm,tz_hour,tz_min)	\
(tm->tm_hour += tz_hour, 		\
 tm->tm_min += tz_min,			\
 mktime(tm_time)) ? 0 : 1

/* setup flags macroses */
#define XO_FLAG_ON(flag)	flag = 1
#define XO_FLAG_OFF(flag)	flag = 0

#define DEBUG_PRINT(x)		printf(x"\n")

#define CHARS_SIZE(char_array)	(sizeof(char_array)/sizeof(char))

/* cache time flags */
#define XO_CACHE_EMPTY		(0<<0)
#define	XO_CACHE_SECOND		(1<<0)
#define XO_CACHE_MINUTE		(1<<1)
#define XO_CACHE_HOUR		(1<<2)
#define XO_CACHE_ROT_AXIS	(1<<3)
#define XO_CACHE_USE_MASK	(1<<4)

/* how many vertex we are used to draw the clock hand */
#define XO_CACHE_POLY_VERTEX	4

/* how many vertex we are used to draw the clock hand head */
#define XO_CACHE_POLY_VERTEX_HEAD	36

#define XO_CACHE_DATE_BUFF_LEN		32

/* clock hand color */
typedef struct clock_hand_color {
	unsigned char red;
	unsigned char green;
	unsigned char blue;
	unsigned char alpha;
} clock_hand_color_t;

/* clock hand color and geometry */
typedef struct clock_hand {
	clock_hand_color_t color;
	unsigned int length;
	unsigned int width;
	unsigned int overlap;
	unsigned int head_diam;
} clock_hand_t;

/* date string params */
typedef struct date_str {
	clock_hand_color_t color;
	char *font_name;
	char *format;
	int pos;
	int offset_h;
	int offset_v;
} date_str_t;

/* what cache need */
typedef struct xo_cache {
	Display *display;	/* main display */
	Window *window;		/* where the clock is drawing */
	Pixmap *pix_skin;	/* clock skin: we use it to
				 * make local copies of the skin */
	Pixmap *pix_mask;	/* clock mask */
	date_str_t * date_string;/* date string params */
	/* clock hands stuff */
	clock_hand_t *hand_hour;
	clock_hand_t *hand_min;
	clock_hand_t *hand_sec;
	unsigned long flags;	/* clock flags */
	XPoint *hand_rot_axis;	/* hands rotation axis coords */
	int tz_hour;		/* time-zone hour shift */
	int tz_min;		/* time-zone minute shift */
} xo_cache_t;

/* internal procedure */
void _cache_flush_vars(void);
int _cache_print_date(Display *display, Visual *visual,
		      Colormap colormap, Pixmap pix_canvas,
		      XftFont *font, XftColor color,
		      unsigned int width, unsigned int height,
		      enum placement base_pos, int offset_h, int offset_v,
		      char *date_string, unsigned int date_string_len);
int _cache_make_hand_head(XPointDouble head_poly[],
			  unsigned long poly_count,
			  XPointDouble axis_zero,
			  unsigned int hand_width);

/* cache interface */
int cache_init(xo_cache_t *cache_params_ptr);
int cache_get_composed(Pixmap *pix_composed_ptr,
		       int update_flag);
void cache_free(void);
int cache_get_gc(GC *gc_ptr);
int cache_is_busy(void);
void * cache_x_date_buf_op(int opcode, char *format);
int cache_update_skin(Pixmap *pix_new,
		      unsigned int width,
		      unsigned int height);

#endif/*CACHE_H_*/
