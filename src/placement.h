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

#ifndef PLACEMENT_H_
#define PLACEMENT_H_

#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <unistd.h>
#include <time.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>


#define XO_HALF(x)		(x / 2)
#define XO_PI			(3.14159265359)
#define XO_DEG_TO_RAD(deg)	(deg * XO_PI / 180.0)
#define XO_RAD_TO_DEG(rad)	(rad * 180.0 / XO_PI)

/* clock base placement */
enum placement {
	TOP_LEFT	= 0,
	TOP_CENTER	= 1,
	TOP_RIGHT	= 2,
	MIDDLE_LEFT	= 3,
	MIDDLE_CENTER	= 4,
	MIDDLE_RIGHT	= 5,
	BOTTOM_LEFT	= 6,
	BOTTOM_CENTER	= 7,
	BOTTOM_RIGHT	= 8
};

/* fields of tm struct to be retreived */
enum time_field {
	SECOND			= 1,
	MINUTE			= 2,
	HOUR			= 3,
	MINUTE_BY_SECOND	= 4,
	HOUR_BY_MINUTE		= 5
};

int get_clock_placement(unsigned int display_width,
			unsigned int display_height,
			unsigned int clock_width,
			unsigned int clock_height,
			enum placement base,
			int offset_h,
			int offset_v,
			unsigned int *dst_x,
			unsigned int *dst_y);
int get_cw_rotated_rect(float degree_ccw,
			XPointDouble zero,
			unsigned int length,
			unsigned int width,
			unsigned int overlap,
			XPointDouble rect_points[]);
int get_handle_angle_on_time(const struct tm *ptm,
			     int tm_field,
			     float *angle);

#endif/*PLACEMENT_H_*/
