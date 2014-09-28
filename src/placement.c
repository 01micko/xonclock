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

#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <unistd.h>
#include <time.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>

#include "placement.h"
#include "xonclock.h"


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	get_clock_placement
 * $Desc:	returns top left coordinates of a clock
 * $In:		display_width - width of a display
 *		display_height - height of a display
 *		clock_width, clock_height - same for the clock
 *		base - clock base placement (see placement.h)
 *		offset_h - horizontal offset (from left to right)
 *		offset_v - vertical offset (from top to bottom)
 * $Out:	dst_x, dst_y - destination coordinates
 * $Ret:	0 on succsess, otherwise -1
 *		and sets dst_... to 0
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int get_clock_placement(unsigned int display_width,
			unsigned int display_height,
			unsigned int clock_width,
			unsigned int clock_height,
			enum placement base,
			int offset_h,
			int offset_v,
			unsigned int *dst_x,
			unsigned int *dst_y)
{
	unsigned int x = 0, y = 0;

	/* check entry */
	if (display_width < 1 || display_height < 1) {
		msg("Bad procedure arguments");
		*dst_x = 0;
		*dst_y = 0;
		return -1;
	}

	switch(base) {
	case TOP_LEFT:
		x = offset_h;
		y = offset_v;
		break;
	case TOP_CENTER:
		x = XO_HALF(display_width) - XO_HALF(clock_width) + offset_h;
		y = offset_v;
		break;
	case TOP_RIGHT:
		x = display_width - clock_width + offset_h;
		y = offset_v;
		break;
	case MIDDLE_LEFT:
		x = offset_h;
		y = XO_HALF(display_height) - XO_HALF(clock_height) + offset_v;
		break;
	case MIDDLE_CENTER:
		x = XO_HALF(display_width) - XO_HALF(clock_width) + offset_h;
		y = XO_HALF(display_height) - XO_HALF(clock_height) + offset_v;
		break;
	case MIDDLE_RIGHT:
		x = display_width - clock_width + offset_h;
		y = XO_HALF(display_height) - XO_HALF(clock_height) + offset_v;
		break;
	case BOTTOM_LEFT:
		x = offset_h;
		y = display_height - clock_height + offset_v;
		break;
	case BOTTOM_CENTER:
		x = XO_HALF(display_width) - XO_HALF(clock_width) + offset_h;
		y = display_height - clock_height + offset_v;
		break;
	case BOTTOM_RIGHT:
		x = display_width - clock_width + offset_h;
		y = display_height - clock_height + offset_v;
		break;
	} /* switch(base) */

	/* check for extrem. cases */
	if (x > display_width) x = 0;
	if (y > display_height) y = 0;

	/* write results */
	*dst_x = x;
	*dst_y = y;

	return 0;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	get_cw_rotated_rect
 * $Desc:	returns clockwise rotated (on arbitrary angle)
 *		rectangle points
 * $Args:	(i) degree_cw - angle (must be positive)
 *		(i) zero - XY axis zero point
 *		(i) length - length of the rectangle
 *		(i) width - width of the rectangle (should be even number)
 *		(i) overlap - length of hand overlapped tail
 *		(o) rect_points - new 4 points
 * $Ret:	0 on success otherwise -1 (not used for now)
 * $Comments:	1) the memory allocated for rect_points MUST be enough to
 *		contain 4 elements of XPointDouble, so rect_points
 *		MUST be declared as XPointDouble rect_points[4] at least
 *		2) the axis direction:
 *                 o----+ x
 *                 |
 *                 |
 *                 +
 *                 y
 *		3) created rectangle will have the following points indexes
 *                 2 +----------------+ 3
 *                   |                |
 *              o ---+----------------+--- o (zero angle)
 *                   |                |
 *                 1 +----------------+ 0
 *		where "o-o" is axis rely on which the rectangle
 *		is forming
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int get_cw_rotated_rect(float degree_ccw, XPointDouble zero,
			unsigned int length, unsigned int width,
			unsigned int overlap,
			XPointDouble rect_points[])
{
	float rad = XO_DEG_TO_RAD(degree_ccw);
	float cosf_ = cosf(rad);
	float sinf_ = sinf(rad);
	float width_ = (float)width / 2.;

	/* rotation */
	rect_points[0].x = length * cosf_ - width_ * sinf_ + zero.x;
	rect_points[0].y = length * sinf_ + width_ * cosf_ + zero.y;

	rect_points[1].x = - width_ * sinf_ + zero.x - overlap * cosf_;
	rect_points[1].y = width_ * cosf_ + zero.y - overlap * sinf_;

	rect_points[2].x = width_ * sinf_ + zero.x - overlap * cosf_;
	rect_points[2].y = - width_ * cosf_ + zero.y - overlap * sinf_;

	rect_points[3].x = length * cosf_ + width_ * sinf(rad) + zero.x;
	rect_points[3].y = length * sinf_ + - width_ * cosf(rad) + zero.y;

	return 0;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	get_handle_angle_on_time
 * $Desc:	computes clock hand rotation angle based upon time
 * $Args:	(i) ptm - time
 *		(i) tm_field - time option flags (see placement.h)
 *		(o) angle - computed angle
 * $Ret:	if OK - 0, otherwise -1
 * $Comments:	1) The clock hand is clockwise rotating.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int get_handle_angle_on_time(const struct tm *ptm,
			     int tm_field,
			     float *angle)
{
	/* check entry */
	if (ptm == NULL) {
		msg("Bad procedure argument");
		return -1;
	}

	switch (tm_field) {
	case SECOND:
		*angle = 360. / 60. * ptm->tm_sec - 90.;
		break;
	case MINUTE:
		*angle = 360. / 60. * ptm->tm_min - 90.;
		break;
	case HOUR:
		if (ptm->tm_hour > 12) {
			*angle = 360. / 12. * (ptm->tm_hour - 12.) - 90;
		} else {
			*angle = 360. / 12. * ptm->tm_hour - 90;
		}
		break;
	case MINUTE_BY_SECOND:
		*angle = 360. / 60. * ptm->tm_min - 90. +
			6. / 60. * ptm->tm_sec;
		break;
	case HOUR_BY_MINUTE:
		if (ptm->tm_hour > 12) {
			*angle = 360. / 12. * (ptm->tm_hour - 12.) - 90 +
				0.5 * ptm->tm_min;
		} else {
			*angle = 360. / 12. * ptm->tm_hour - 90 +
				0.5 * ptm->tm_min;
		}
		break;
	default:
		msg("Unable to select time field");
		return -1;
		break;
	} /* switch (tm_field) */

	return 0;
}
