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

#ifndef IMAGE_H_
#define IMAGE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <unistd.h>
#include <time.h>
#include <getopt.h>
#include <signal.h>
#include <sys/time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xrender.h>
#include <X11/Xft/Xft.h>


#define ALPHA_TO_DEC(alpha) (alpha / 255.0)
#define COLOR_MESS(fore_color, back_color, alpha)	\
	((unsigned long)(back_color * (1.0 - alpha)) +	\
	(unsigned long)(fore_color * alpha))

/* image formats */
#define IFMT_UNK	-1
#define IFMT_PNG	1
#define IFMT_XPM	2
#define IFMT_TIF	3
#define IFMT_JPG	4

void color_shift(unsigned long mask,
		 unsigned char * left_shift,
		 unsigned char * right_shift);

int pixmap_from_rgb(Display *dpy,
		    Window *win,
		    GC *gc,
		    Visual *visual,
		    unsigned int depth,
		    unsigned char * rgb,
		    unsigned int width,
		    unsigned int height,
		    Pixmap *pixmap);

int mask_from_alpha(Display *dpy,
		    Window *win,
		    Visual *vis,
		    unsigned int depth,
		    unsigned char * alpha,
		    unsigned int width,
		    unsigned int height,
		    unsigned int threshold,
		    Pixmap *pixmap);

int get_image_type_from_ext(char *path);

unsigned char * get_dpy_bg_rgb(Display *dpy,
			       unsigned char *rgb_prealloced,
			       int pos_x,
			       int pos_y,
			       unsigned int width,
			       unsigned int height);

int merge_rgba_aa(unsigned char *rgb_bg,
		  unsigned char *rgb_fg,
		  unsigned char *alfa_bg,
		  unsigned char *alfa_fg,
		  unsigned char *rgb_merged,
		  unsigned char *alfa_merged,
		  int width,
		  int height);

unsigned char * get_rgb_from_ximage(Display *dpy,
				    Visual *vis,
				    XImage *image,
				    unsigned char *rgb_prealloced,
				    unsigned int width,
				    unsigned int height);

#endif/*IMAGE_H_*/
