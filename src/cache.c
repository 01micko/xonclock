/* -*- mode: linux-c -*-
 *
 *      Copyright (C) 2005-2006  Cyrill Gorcunov <gclion@mail.ru>
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
#include <unistd.h>
#include <math.h>
#include <limits.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>

#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>
#include <X11/Xft/Xft.h>

#include "cache.h"
#include "placement.h"
#include "xonclock.h"

static Display *display;		/* local copy of main display */
static Window window;			/* local copy of clock window */
static XWindowAttributes win_attribs;	/* its attributes */

static GC gc;				/* our GC */
static unsigned long ul_flags = 0;
static XGCValues gc_vals;

static XPointDouble poly[XO_CACHE_POLY_VERTEX];	/* here is clock hand points */
static XPointDouble zero_point;		/* where the clock hand begin */

/* clock hand heads vertexes */
static XPointDouble poly_sec_head[XO_CACHE_POLY_VERTEX_HEAD];
static XPointDouble poly_min_head[XO_CACHE_POLY_VERTEX_HEAD];
static XPointDouble poly_hour_head[XO_CACHE_POLY_VERTEX_HEAD];

static XRenderPictFormat* xre_pict_format = NULL;

/* clock hands colors and geometry */
static XRenderColor xre_color_sec, xre_color_min, xre_color_hour;
static XftColor xft_color_sec, xft_color_min, xft_color_hour;
static clock_hand_t hand_hour, hand_min, hand_sec;

static Pixmap pix_virgin;		/* used if build date string */
static Pixmap pix_skin;			/* clock skin pixmap */
static Pixmap pix_mask;			/* its mask */
static Pixmap pix_hands;		/* pixmap for clock hands */
static Pixmap pix_cache;		/* composed clock pixmap */
static Pixmap pix_update;		/* for clock skin updating */

/* Xft and Xrender stuffs to draw smooth clock hands */
static XftDraw *draw_hands;
static XftDraw *draw_skin;

static Picture pic_skin;
static Picture pic_hour;
static Picture pic_min;
static Picture pic_sec;

/* structres that needed to retrieve current time */
static time_t *ttime = NULL;
static time_t cur_time;
static struct tm* tm_time = NULL;
static struct tm  tm_time_printed = {};

static float hand_angle;		/* clock hand slope */
static unsigned long cache_flags = XO_CACHE_EMPTY;

/* date string stuff */
static int show_date = 0;
static XftFont *xft_date_font = NULL;
static XftColor xft_date_color;
static XRenderColor xre_date_color;
static char date_buf[XO_CACHE_DATE_BUFF_LEN];
static unsigned int date_buf_len = 0;
static date_str_t *date_string = NULL;

/* to copy date string in X buffer */
static char x_date_buf[XO_CACHE_DATE_BUFF_LEN];
static unsigned int x_date_buf_len = 0;

/* to free allocated resource properly we set the appropriate
 * flag when one of resources is allocated */
static char sf_gc = 0;
static char sf_xft_color_sec = 0;
static char sf_xft_color_min = 0;
static char sf_xft_color_hour = 0;
static char sf_pix_skin = 0;
static char sf_pix_virgin = 0;
static char sf_pix_cache = 0;
static char sf_pix_update = 0;
static char sf_pix_hands = 0;
static char sf_draw_skin = 0;
static char sf_draw_hands = 0;
static char sf_xft_date_color = 0;
static char sf_xft_date_font = 0;

/* internal code flow controlling vars */
static volatile sig_atomic_t cache_initialized = 0;
static volatile sig_atomic_t cache_critical_op = 0;
static volatile sig_atomic_t cache_have_composed = 0;
static volatile sig_atomic_t cache_draw_hand_heads = 0;
static volatile sig_atomic_t cache_x_date_buf_lock = 0;
static volatile sig_atomic_t cache_updating = 0;
static volatile sig_atomic_t cache_update_query = 0;

/* to custom time-zone */
static int tz_hour = 0;
static int tz_min = 0;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	_cache_flush_vars
 * $Desc:	resets cache variables
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void _cache_flush_vars(void)
{
	display = NULL;

	xo_bzero(&window, sizeof(window));
	xo_bzero(&win_attribs, sizeof(win_attribs));
	xo_bzero(&gc, sizeof(gc));
	ul_flags = 0;
	xo_bzero(&gc_vals, sizeof(gc_vals));

	xo_bzero(&poly, (sizeof(poly) / sizeof(XPointDouble)));
	xo_bzero(&poly, sizeof(zero_point));

	xre_pict_format = NULL;

	xo_bzero(&xre_color_sec, sizeof(xre_color_sec));
	xo_bzero(&xre_color_min, sizeof(xre_color_min));
	xo_bzero(&xre_color_hour, sizeof(xre_color_hour));
	xo_bzero(&xft_color_sec, sizeof(xft_color_sec));
	xo_bzero(&xft_color_min, sizeof(xft_color_min));
	xo_bzero(&xft_color_hour, sizeof(xft_color_hour));

	xo_bzero(&hand_sec, sizeof(hand_sec));
	xo_bzero(&hand_min, sizeof(hand_min));
	xo_bzero(&hand_hour, sizeof(hand_hour));

	xo_bzero(&pix_virgin, sizeof(pix_virgin));
	xo_bzero(&pix_skin, sizeof(pix_skin));
	xo_bzero(&pix_cache, sizeof(pix_cache));
	xo_bzero(&pix_mask, sizeof(pix_mask));
	xo_bzero(&pix_hands, sizeof(pix_hands));

	draw_hands = NULL;
	draw_skin = NULL;

	ttime = NULL;
	xo_bzero(&cur_time, sizeof(cur_time));
	tm_time = NULL;

	hand_angle = 0.f;
	cache_flags = XO_CACHE_EMPTY;

	XO_FLAG_OFF(sf_gc);

	XO_FLAG_OFF(sf_pix_skin);
	XO_FLAG_OFF(sf_pix_virgin);
	XO_FLAG_OFF(sf_pix_update);
	XO_FLAG_OFF(sf_pix_cache);
	XO_FLAG_OFF(sf_pix_hands);

	XO_FLAG_OFF(sf_xft_color_sec);
	XO_FLAG_OFF(sf_xft_color_min);
	XO_FLAG_OFF(sf_xft_color_hour);

	XO_FLAG_OFF(sf_draw_skin);
	XO_FLAG_OFF(sf_draw_skin);

	XO_FLAG_OFF(show_date);
	xft_date_font = NULL;
	date_buf_len = 0;
	date_string = NULL;
	xo_bzero(&xft_date_color, sizeof(xft_date_color));
	xo_bzero(&xre_date_color, sizeof(xre_date_color));
	xo_bzero(date_buf, CHARS_SIZE(date_buf));
	XO_FLAG_OFF(sf_xft_date_color);
	XO_FLAG_OFF(sf_xft_date_font);

	XO_FLAG_OFF(cache_initialized);
	XO_FLAG_OFF(cache_critical_op);
	XO_FLAG_OFF(cache_have_composed);
	XO_FLAG_OFF(cache_draw_hand_heads);
	XO_FLAG_OFF(cache_updating);
	XO_FLAG_OFF(cache_update_query);

	xo_bzero(&tm_time_printed, sizeof(struct tm));

	XO_FLAG_OFF(cache_x_date_buf_lock);
	xo_bzero(x_date_buf, XO_CACHE_DATE_BUFF_LEN);
	x_date_buf_len = 0;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	cache_init
 * $Desc:	initializates cache structures
 * $Args:	(i) cache_params_ptr - pointer to the cache parameters
 *		structure
 * $Ret:	0 on success, othrewise -1
 * $Comments:	1) The cache_params->window, cache_params->pix_skin
 *		and cache_params->pix_mask must have same sizes (width
 *		and height) otherwise the procedure behaiviour is
 *		unpredictable.
 *		2) While setting up some parameter we set an appropriate
 *		'setup flag' (named as sf_...) which'll be used in
 *		cache_free() procedure.
 *		3) If date string is used then the original clock skin
 *		poited by cache_params_ptr->pix_skin will be composed
 *		with date string, so if you planning to use original
 *		skin pixmap outside of cache the skin pixmap copy should
 *		be passed into the cache
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int cache_init(xo_cache_t *cache_params_ptr)
{
	/* check entry */
	if (cache_params_ptr == NULL) {
		msg("NULL pointer passed");
		cache_free(); /* -> below */
		return -1;
	}

	/* check the inlet pointers */
	if (cache_params_ptr->display == NULL		||
	    cache_params_ptr->window == NULL		||
	    cache_params_ptr->pix_skin == NULL		||
	    (cache_params_ptr->pix_mask == NULL &&
	     cache_params_ptr->flags & XO_CACHE_USE_MASK) ||
	    cache_params_ptr->hand_hour == NULL		||
	    cache_params_ptr->hand_min == NULL		||
	    cache_params_ptr->hand_sec == NULL		||
	    cache_params_ptr->flags == XO_CACHE_EMPTY) {
		msg("NULL pointer passed");
		cache_free(); /* -> below */
		return -1;
	}

	/* rotation axis */
	if (cache_params_ptr->flags & XO_CACHE_ROT_AXIS) {
		if (cache_params_ptr->hand_rot_axis == NULL) {
			msg("NULL pointer passed");
			cache_free(); /* -> below */
			return -1;
		}
	}

	/* checking for date string params */
	if (cache_params_ptr->date_string) {
		if (cache_params_ptr->date_string->font_name == NULL ||
		    cache_params_ptr->date_string->format == NULL) {
			msg("NULL pointer passed");
			cache_free(); /* -> below */
			return -1;
		}
		xre_date_color.red =
			cache_params_ptr->date_string->color.red << 8;
		xre_date_color.green =
			cache_params_ptr->date_string->color.green << 8;
		xre_date_color.blue =
			cache_params_ptr->date_string->color.blue << 8;
		xre_date_color.alpha =
			cache_params_ptr->date_string->color.alpha << 8;
		XO_FLAG_ON(show_date);
	} else {
		XO_FLAG_OFF(show_date);
	}

	/* save some vars in our local copies */
	display = cache_params_ptr->display;
	window = *cache_params_ptr->window;
	XGetWindowAttributes(display, window, &win_attribs);
	if (cache_params_ptr->flags & XO_CACHE_USE_MASK)
		pix_mask = *cache_params_ptr->pix_mask;
	cache_flags = cache_params_ptr->flags;
	if (cache_flags & XO_CACHE_SECOND)
		hand_sec = *cache_params_ptr->hand_sec;
	if (cache_flags & XO_CACHE_MINUTE)
		hand_min = *cache_params_ptr->hand_min;
	if (cache_flags & XO_CACHE_HOUR)
		hand_hour = *cache_params_ptr->hand_hour;
	tz_hour = cache_params_ptr->tz_hour;
	tz_min = cache_params_ptr->tz_min;

	/* make the pixmaps we need */
	pix_skin = XCreatePixmap(display, window,
				 win_attribs.width,
				 win_attribs.height,
				 win_attribs.depth);
	pix_virgin = XCreatePixmap(display, window,
				   win_attribs.width,
				   win_attribs.height,
				   win_attribs.depth);
	pix_hands = XCreatePixmap(display, window,
				  win_attribs.width,
				  win_attribs.height,
				  win_attribs.depth);
	pix_update = XCreatePixmap(display, window,
				   win_attribs.width,
				   win_attribs.height,
				   win_attribs.depth);
	XO_FLAG_ON(sf_pix_skin);
	XO_FLAG_ON(sf_pix_virgin);
	XO_FLAG_ON(sf_pix_hands);
	XO_FLAG_ON(sf_pix_update);

	/* make own GC */
	ul_flags = GCFunction | GCClipXOrigin | GCClipYOrigin;
	gc_vals.function = GXcopy;
	gc_vals.clip_x_origin = 0;
	gc_vals.clip_y_origin = 0;
	if (cache_flags & XO_CACHE_USE_MASK) {
		ul_flags |= GCClipMask;
		gc_vals.clip_mask = pix_mask;
	}
	gc = XCreateGC(display, window, ul_flags, &gc_vals);
	XO_FLAG_ON(sf_gc);

	/* copy original clock skin in our pixmaps */
	XCopyArea(display, *cache_params_ptr->pix_skin, pix_skin, gc,
		  0, 0, win_attribs.width, win_attribs.height, 0, 0);
	XCopyArea(display, *cache_params_ptr->pix_skin, pix_virgin, gc,
		  0, 0, win_attribs.width, win_attribs.height, 0, 0);

	/* allocate clock hands colors */
	if (cache_flags & XO_CACHE_SECOND) {
		xre_color_sec.red = (hand_sec.color.red << 8);
		xre_color_sec.green = (hand_sec.color.green << 8);
		xre_color_sec.blue = (hand_sec.color.blue << 8);
		xre_color_sec.alpha = (hand_sec.color.alpha << 8);
		if (XftColorAllocValue(display,
				       win_attribs.visual,
				       win_attribs.colormap,
				       &xre_color_sec,
				       &xft_color_sec) == False) {
			msg("Unable to allocate color"
			    " for a clock second hand");
			cache_free(); /* -> below */
			return -1;
		}
		XO_FLAG_ON(sf_xft_color_sec);
	}
	if (cache_flags & XO_CACHE_MINUTE) {
		xre_color_min.red = (hand_min.color.red << 8);
		xre_color_min.green = (hand_min.color.green << 8);
		xre_color_min.blue = (hand_min.color.blue << 8);
		xre_color_min.alpha = (hand_min.color.alpha << 8);
		if (XftColorAllocValue(display,
				       win_attribs.visual,
				       win_attribs.colormap,
				       &xre_color_min,
				       &xft_color_min) == False) {
			msg("Unable to allocate color"
				" for a clock minute hand");
			cache_free(); /* -> below */
			return -1;
		}
		XO_FLAG_ON(sf_xft_color_sec);
	}
	if (cache_flags & XO_CACHE_HOUR) {
		xre_color_hour.red = (hand_hour.color.red << 8);
		xre_color_hour.green = (hand_hour.color.green << 8);
		xre_color_hour.blue = (hand_hour.color.blue << 8);
		xre_color_hour.alpha = (hand_hour.color.alpha << 8);
		if (XftColorAllocValue(display,
				       win_attribs.visual,
				       win_attribs.colormap,
				       &xre_color_hour,
				       &xft_color_hour) == False) {
			msg("Unable to allocate color"
			    " for a clock hour hand");
			cache_free(); /* -> below */
			return -1;
		}
		XO_FLAG_ON(sf_xft_color_sec);
	}

	if (show_date) {
		/* allocate color for the date string */
		if (XftColorAllocValue(display,
				       win_attribs.visual,
				       win_attribs.colormap,
				       &xre_date_color,
				       &xft_date_color) == False) {
			msg("Unable to allocate color"
			    " for a date string");
			cache_free(); /* -> below */
			return -1;
		}
		XO_FLAG_ON(sf_xft_date_color);

		/* open font for the date string */
		xft_date_font = XftFontOpenName(display,
						DefaultScreen(display),
						cache_params_ptr->date_string->font_name);
		if (xft_date_font == NULL) {
			msg("Unable to open font specified"
			    " for a date string");
			cache_free(); /* -> below */
			return -1;
		}
		XO_FLAG_ON(sf_xft_date_font);

		/* date string drawing */
		cur_time = time(ttime);
		tm_time = localtime(&cur_time);
		if (tz_hour || tz_min) {
			if (tz_update(tm_time, tz_hour, tz_min)) {
				msg("Unable to create a date string with "
				    "custom TZ");
				tm_time = localtime(&cur_time);
			}
		}
		tm_time_printed = *tm_time; /* save printed */
		date_buf_len = strftime(date_buf, CHARS_SIZE(date_buf),
					cache_params_ptr->date_string->format,
					tm_time);
		if (date_buf_len < 1) {
			msg("Unable to create a date string");
		} else {
			date_string = cache_params_ptr->date_string;
			if (_cache_print_date(display, win_attribs.visual,
					      win_attribs.colormap, pix_skin,
					      xft_date_font, xft_date_color,
					      win_attribs.width,
					      win_attribs.height,
					      date_string->pos,
					      date_string->offset_h,
					      date_string->offset_v,
					      date_buf, date_buf_len) == -1) {
				msg("Unable to draw a date string");
			}
		}
	} /* if (show_date) */


	/* where the clock hands begin */
	if (cache_params_ptr->flags & XO_CACHE_ROT_AXIS) {
		zero_point.x = cache_params_ptr->hand_rot_axis->x;
		zero_point.y = cache_params_ptr->hand_rot_axis->y;
	} else {
		zero_point.x = win_attribs.width / 2;
		zero_point.y = win_attribs.height / 2;
	}

	/* make the pixmap we use while compositing the clock */
	pix_cache = XCreatePixmap(display, window,
				  win_attribs.width,
				  win_attribs.height,
				  win_attribs.depth);
	XO_FLAG_ON(sf_pix_cache);

	/* copy original clock skin in our pixmap */
	XCopyArea(display, pix_skin, pix_cache, gc,
		  0, 0, win_attribs.width, win_attribs.height, 0, 0);

	/* get Xrender picture fomat */
	xre_pict_format = XRenderFindStandardFormat(display,
						    PictStandardA8);
	if (xre_pict_format == NULL) {
		msg("Unable to find Xrender picture format");
		cache_free(); /* -> below */
		return -1;
	}

	/* make Xft and Xrender stuff */
	draw_skin = XftDrawCreate(display,
				  pix_cache,
				  win_attribs.visual,
				  win_attribs.colormap);
	if (draw_skin == NULL) {
		msg("Unable to create Xft draw object");
		cache_free(); /* -> below */
		return -1;
	}
	XO_FLAG_ON(sf_draw_skin);
	pic_skin = XftDrawPicture(draw_skin);
	draw_hands = XftDrawCreate(display,
				   pix_hands,
				   win_attribs.visual,
				   win_attribs.colormap);
	if (draw_hands == NULL) {
		msg("Unable to create Xft draw object");
		cache_free(); /* -> below */
		return -1;
	}
	XO_FLAG_ON(sf_draw_hands);
	if (cache_flags & XO_CACHE_SECOND) {
		pic_sec = XftDrawSrcPicture(draw_hands, &xft_color_sec);
	}
	if (cache_flags & XO_CACHE_MINUTE) {
		pic_min = XftDrawSrcPicture(draw_hands, &xft_color_min);
	}
	if (cache_flags & XO_CACHE_HOUR) {
		pic_hour = XftDrawSrcPicture(draw_hands, &xft_color_hour);
	}

	/* compute vertexes for clock hands heads --> below */
	if (_cache_make_hand_head(poly_sec_head,
				  XO_CACHE_POLY_VERTEX_HEAD,
				  zero_point,
				  cache_params_ptr->hand_sec->width) == 0 &&
	    _cache_make_hand_head(poly_min_head,
				  XO_CACHE_POLY_VERTEX_HEAD,
				  zero_point,
				  cache_params_ptr->hand_min->width) == 0 &&
	    _cache_make_hand_head(poly_hour_head,
				  XO_CACHE_POLY_VERTEX_HEAD,
				  zero_point,
				  cache_params_ptr->hand_hour->width) == 0) {
		XO_FLAG_ON(cache_draw_hand_heads);
	} else {
		XO_FLAG_OFF(cache_draw_hand_heads);
	}


	/* well, cache is initialized */
	XO_FLAG_ON(cache_initialized);

	/* now make the first composed clock pixmap */
	cache_get_composed(NULL, 1);
	XO_FLAG_ON(cache_have_composed);

	return 0;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	cache_get_gc
 * $Desc:	returns cache inetrnal GC
 * $Args:	(o) gc_ptr - pointer to internal GC
 * $Ret:	0 - all OK, -1 - cache init. incomplete
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int cache_get_gc(GC *gc_ptr)
{
	if (cache_initialized == 0) {
		msg("Call cache_init first");
		return -1;
	}
	if (gc_ptr != NULL)
		*gc_ptr = gc;
	return 0;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	cache_is_busy
 * $Desc:	checks for cache critical operation
 * $Ret:	1 - cache is busy, 0 - cache may be used
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int cache_is_busy(void)
{
	return cache_critical_op;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	cache_get_composed
 * $Desc:	returns composed clock pixmap
 * $Args:	(o) pix_composed_ptr - pointer to composed pixmap
 *		(i) update_flag - if set to 1 then cache updates composed
 *		pixmap properly with current time, otherwise it
 *		returns previously composed pixmap
 * $Ret:	0 on success, othrewise -1 (don't use any function
 *		if you've received -1, it means that system error
 *		occured and you have to reinitialize cache anew, if
 *		it returns -2 it means that procedure is being executed
 *		already
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int cache_get_composed(Pixmap *pix_composed_ptr, int update_flag)
{
	int update_date = 0;

	/* cache must be initialized */
	if (!cache_initialized) {
		msg("Call cache_init first");
		return -1;
	}

	/* it's being executed */
	if (cache_critical_op) {
		msg("Cache is busy");
		return -2;
	}

	XO_FLAG_ON(cache_critical_op);

	/* have we to update a skin */
	if (cache_update_query) {
		update_flag = 1; /* do not return cached pixmap */
		XCopyArea(display, pix_update, pix_virgin, gc,
			  0, 0, win_attribs.width,
			  win_attribs.height,
			  0, 0);
		XCopyArea(display, pix_virgin, pix_skin, gc,
			  0, 0, win_attribs.width,
			  win_attribs.height,
			  0, 0);
		update_date = 1;
		XO_FLAG_OFF(cache_update_query);
	}

	/* return previously composed pixmap */
	if (update_flag != 1) {
		if (pix_composed_ptr != NULL) {
			*pix_composed_ptr =  XftDrawDrawable(draw_skin);
			XO_FLAG_OFF(cache_critical_op);
			return 0;
		} else {
			msg("Call cache_init first");
			XO_FLAG_OFF(cache_critical_op);
			return -1;
		}
	}

	/* new time */
	cur_time = time(ttime);
	tm_time = localtime(&cur_time);
	if (tz_hour || tz_min) {
		if (tz_update(tm_time, tz_hour, tz_min)) {
			msg("Unable to create a date string with "
			    "custom TZ");
			tm_time = localtime(&cur_time);
		}
	}

	/* maybe we should make new date */
	if (show_date) {
		if (tm_time->tm_mday != tm_time_printed.tm_mday		||
		    tm_time->tm_mon != tm_time_printed.tm_mon		||
		    tm_time->tm_year != tm_time_printed.tm_year		||
		    update_date) {
			/* restore virgin skin */
			XCopyArea(display, pix_virgin, pix_skin, gc,
				  0, 0, win_attribs.width,
				  win_attribs.height,
				  0, 0);
			/* make new date */
			tm_time_printed = *tm_time;
			date_buf_len = strftime(date_buf, CHARS_SIZE(date_buf),
						date_string->format,
						tm_time);
			if (date_buf_len < 1) {
				msg("Unable to create a date string");
			} else {
				if (_cache_print_date(display,
						      win_attribs.visual,
						      win_attribs.colormap,
						      pix_skin,
						      xft_date_font,
						      xft_date_color,
						      win_attribs.width,
						      win_attribs.height,
						      date_string->pos,
						      date_string->offset_h,
						      date_string->offset_v,
						      date_buf, date_buf_len) == -1) {
					msg("Unable to draw a date string");
				}
			}
		} /* tm_time != tm_time_printed */
	} /* if (show_date) */

	/* not first call - release dirty objects */
	if (cache_have_composed == 1) {
		XftDrawDestroy(draw_skin); /* it also will free pic_skin */
		XCopyArea(display, pix_skin, pix_cache, gc,
			  0, 0, win_attribs.width, win_attribs.height,
			  0, 0);
		draw_skin = XftDrawCreate(display,
					  pix_cache,
					  win_attribs.visual,
					  win_attribs.colormap);
		if (draw_skin == NULL) {
			msg("Unable to create Xft object");
			XO_FLAG_OFF(cache_critical_op);
			cache_free(); /* below */
			return -1;
		}
		pic_skin = XftDrawPicture(draw_skin);
	} /* cache_have_composed = 1 */

	/* COMPOSITING */
	if (cache_flags & XO_CACHE_HOUR) {
		if (cache_flags & XO_CACHE_MINUTE) {
			/* -> placement.c */
			get_handle_angle_on_time(tm_time,
						 HOUR_BY_MINUTE,
						 &hand_angle);
		} else {
			/* -> placement.c */
			get_handle_angle_on_time(tm_time,
						 HOUR,
						 &hand_angle);
		}
		get_cw_rotated_rect(hand_angle, zero_point,
				    hand_hour.length, hand_hour.width,
				    hand_hour.overlap,
				    poly); /* -> placement.c */
		XRenderCompositeDoublePoly(display, PictOpOver,
					   pic_hour,
					   pic_skin,
					   xre_pict_format,
					   0, 0, 0, 0,
					   poly, XO_CACHE_POLY_VERTEX,
					   0);
		if (cache_draw_hand_heads == 1) {
			XRenderCompositeDoublePoly(display, PictOpOver,
						   pic_hour,
						   pic_skin,
						   xre_pict_format,
						   0, 0, 0, 0,
						   poly_hour_head,
						   XO_CACHE_POLY_VERTEX_HEAD,
						   0);
		}
	} /* cache_flags & XO_CACHE_HOUR */
	if (cache_flags & XO_CACHE_MINUTE) {
		if (cache_flags & XO_CACHE_SECOND) {
			/* -> placement.c */
			get_handle_angle_on_time(tm_time,
						 MINUTE_BY_SECOND,
						 &hand_angle);
		} else {
			/* -> placement.c */
			get_handle_angle_on_time(tm_time,
						 MINUTE,
						 &hand_angle);
		}
		get_cw_rotated_rect(hand_angle, zero_point,
				    hand_min.length, hand_min.width,
				    hand_min.overlap,
				    poly); /* -> placement.c */
		XRenderCompositeDoublePoly(display, PictOpOver,
					   pic_min,
					   pic_skin,
					   xre_pict_format,
					   0, 0, 0, 0,
					   poly, XO_CACHE_POLY_VERTEX,
					   0);
		if (cache_draw_hand_heads == 1) {
			XRenderCompositeDoublePoly(display, PictOpOver,
						   pic_min,
						   pic_skin,
						   xre_pict_format,
						   0, 0, 0, 0,
						   poly_min_head,
						   XO_CACHE_POLY_VERTEX_HEAD,
						   0);
		}
	} /* cache_flags & XO_CACHE_MINUTE */
	if (cache_flags & XO_CACHE_SECOND) {
		get_handle_angle_on_time(tm_time, SECOND,
					 &hand_angle); /* -> placement.c */
		get_cw_rotated_rect(hand_angle, zero_point,
				    hand_sec.length, hand_sec.width,
				    hand_sec.overlap,
				    poly); /* -> placement.c */
		XRenderCompositeDoublePoly(display, PictOpOver,
					   pic_sec,
					   pic_skin,
					   xre_pict_format,
					   0, 0, 0, 0,
					   poly, XO_CACHE_POLY_VERTEX,
					   0);
		if (cache_draw_hand_heads == 1) {
			XRenderCompositeDoublePoly(display, PictOpOver,
						   pic_sec,
						   pic_skin,
						   xre_pict_format,
						   0, 0, 0, 0,
						   poly_sec_head,
						   XO_CACHE_POLY_VERTEX_HEAD,
						   0);
		}
	} /* cache_flags & XO_CACHE_SECOND */

	/* results int pointer */
	if (pix_composed_ptr != NULL)
		*pix_composed_ptr = XftDrawDrawable(draw_skin);

	XO_FLAG_OFF(cache_critical_op);
	return 0;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	_cache_print_date
 * $Desc:	prints date string on the pixmap canvas
 * $Args:	(i) display, visual, colormap - things associateded with
 *		the pix_canvas
 *		(io) pix_canvas - pixmap where we drawing date string
 *		(i) font - font we use to print
 *		(i) color - and its color
 *		(i) width, height - pix_canvas sizes
 *		(i) base_pos - base position of the date string
 *			       (see placement.h)
 *		(i) offset_h, offset_v - date string position offsets
 *		(i) date_string - date string itself
 *		(i) date_string_len - and its length
 * $Ret:	0 - on success, -1 on error
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int _cache_print_date(Display *display, Visual *visual,
		      Colormap colormap, Pixmap pix_canvas,
		      XftFont *font, XftColor color,
		      unsigned int width, unsigned int height,
		      enum placement base_pos, int offset_h, int offset_v,
		      char* date_string, unsigned int date_string_len)
{
	XftDraw *draw = NULL;
	XGlyphInfo extents = {};
	int dst_x = 0, dst_y = 0;

	/* check entry */
	if (display == NULL || visual == NULL ||
	    font == NULL || date_string == NULL) {
		msg("NULL pointer passed");
		return -1;
	}
	if (date_string_len < 1) {
		msg("Date string is too small");
		return -1;
	}
	if (width < 1 || height < 1) {
		msg("Canvas is too small");
		return -1;
	}

	/* get date string geometry */
	XftTextExtents8(display, font,
			(FcChar8*)date_string,
			date_string_len,
			(XGlyphInfo*)&extents);

	/* where we'll print the date string */
	draw = XftDrawCreate(display, pix_canvas,
			     visual, colormap);
	if (draw == NULL) {
		msg("Unable to create Xft draw object");
		return -1;
	}

	/* get date string destination coords,
	 * get_clock_placement is pretty enough
	 * to do such work */
	if (get_clock_placement(width, height,
				extents.width, extents.height,
				base_pos, offset_h, offset_v,
				&dst_x, &dst_y) == -1) {
		msg("Unable to compute date string destination coords");
		XftDrawDestroy(draw);
		return -1;
	}

	/* ok, lets print the date string */
	XftDrawString8(draw, &color, font,
		       dst_x, dst_y + extents.height,
		       (FcChar8*)date_string, date_string_len);

	/* release Xft draw object, we've our date string
	 * on the pix_canvas */
	XftDrawDestroy(draw);
	return 0;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	cache_free
 * $Desc:	freeing cache structures
 * $Comments:	before call this routine please check the cache state
 *		using cache_is_busy() so cache MUST NOT be in
 *		critical state
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void cache_free(void)
{
	/* nothing to free */
	if (!cache_initialized) {
		msg("Call cache_init first");
		return;
	}

	/* to free some resource it's needed to
	 * check an appropriate flag */

	if (sf_draw_hands)
		XftDrawDestroy(draw_hands);
	if (sf_draw_skin)
		XftDrawDestroy(draw_skin);
	if (sf_pix_skin)
		XFreePixmap(display, pix_skin);
	if (sf_pix_virgin)
		XFreePixmap(display, pix_virgin);
	if (sf_pix_hands)
		XFreePixmap(display, pix_hands);
	if (sf_pix_update)
		XFreePixmap(display, pix_update);
	if (sf_pix_cache)
		XFreePixmap(display, pix_cache);
	if (sf_xft_color_hour)
		XftColorFree(display,
			     win_attribs.visual,
			     win_attribs.colormap,
			     &xft_color_hour);
	if (sf_xft_color_min)
		XftColorFree(display,
			     win_attribs.visual,
			     win_attribs.colormap,
			     &xft_color_min);
	if (sf_xft_color_sec)
		XftColorFree(display,
			     win_attribs.visual,
			     win_attribs.colormap,
			     &xft_color_sec);
	if (sf_xft_date_color)
		XftColorFree(display,
			     win_attribs.visual,
			     win_attribs.colormap,
			     &xft_date_color);
	if (sf_xft_date_font)
		XftFontClose(display, xft_date_font);
	if (sf_gc)
		XFreeGC(display, gc);

	/* now flush the variables itself */
	_cache_flush_vars(); /* -> above */
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	_cache_make_hand_head
 * $Desc:	computes circle vertex for a hand rotation head
 * $Args:	(o) head_poly - array of computed vertexes
 *		(i) poly_count - amount of vertex in a circle
 *		(i) axis_zero - coords of hand rotation axis
 *		(i) hand_width - width of a clock hand
 * $Ret:	-1 on error, otherwise 0
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int _cache_make_hand_head(XPointDouble head_poly[],
			  unsigned long poly_count,
			  XPointDouble axis_zero,
			  unsigned int hand_width)
{
	/* check for input */
	if (head_poly == NULL ||
	    poly_count < 2) {
		msg("Bad input arguments");
		return -1;
	}

	/* fixing the width */
	if (hand_width < 1)
		hand_width = 1;

	float sec_angle = XO_DEG_TO_RAD((360. / (float)poly_count));
	float angle = 0.;
	unsigned int radius = 0;
	unsigned long j = 0;
	int i = 0;

	radius = hand_width / 2;
	if (radius < 1) {
		radius = 2;
	} else {
		radius += 2;
	}

	i = poly_count;

	/* OK, let fill our circle */
	angle = 0.; j= 0;
	while(i-- >= 0) {
		head_poly[j].x = axis_zero.x + cosf(angle) * (float)radius;
		head_poly[j].y = axis_zero.y + sinf(angle) * (float)radius;
		j++; angle += sec_angle;
	}

	return 0;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	cache_x_date_buf_op
 * $Desc:	return or update a date string to be pasted in X clipboard
 *		buffer
 * $Args:	(i) opcode:
 *			0 - get a date string  (format arg isn't used)
 *			1 - update string
 *		(i) format - format of a date string (see man strftime)
 * $Ret:	NULL on error or pointer to a date string
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void * cache_x_date_buf_op(int opcode, char *format)
{
	time_t *ttime = NULL;
	time_t cur_time;
	struct tm *tm_time = NULL;

	/* check for reentering */
	if (cache_x_date_buf_lock == 1) {
		return NULL;
	}

	XO_FLAG_ON(cache_x_date_buf_lock);

	switch(opcode) {
	case 1: /* update a date string */
		if (format == NULL) {
			msg("The format of a date string is empty");
			XO_FLAG_OFF(cache_x_date_buf_lock);
			return NULL;
		}
		cur_time = time(ttime);
		tm_time = localtime(&cur_time);
		if (tz_hour || tz_min) {
			if (tz_update(tm_time, tz_hour, tz_min)) {
				msg("Unable to create a date string with "
				    "custom TZ");
				tm_time = localtime(&cur_time);
			}
		}
		x_date_buf_len = strftime(x_date_buf, CHARS_SIZE(x_date_buf),
					  format, tm_time);
		if (x_date_buf_len < 1) {
			msg("Unable to create a date string");
			XO_FLAG_OFF(cache_x_date_buf_lock);
			return NULL;
		}
		break;
	default:
		break;
	}

	XO_FLAG_OFF(cache_x_date_buf_lock);
	return x_date_buf;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	cache_update_skin
 * $Desc:	just saves a new clock skin for virgin pixmap and
 *		sets update query flag
 * $Args:	(i) pix_new - new data for virgin pixmap
 *		(i) width, height - new pixmap sizes (a little prevention
 *		    from setting of bad pixmaps)
 * $Ret:	1 on success, otherwise 0
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int cache_update_skin(Pixmap *pix_new,
		      unsigned int width,
		      unsigned int height)
{
	/* nothing to free */
	if (!cache_initialized) {
		msg("Call cache_init first");
		return 0;
	}
	/* it's being executed */
	if (cache_critical_op) {
		msg("Cache is busy");
		return 0;
	}
	/* already here */
	if (cache_updating)
		return 0;

	/* check data */
	if (width != win_attribs.width ||
	    height !=  win_attribs.height ||
	    !pix_new) {
		msg("Inappropriate pixmap sizes or NULL pointer");
		return 0;
	}

	XO_FLAG_ON(cache_updating);
	/* copy original clock skin in our pixmaps for updateing */
	XCopyArea(display, *pix_new, pix_update, gc,
		  0, 0, win_attribs.width, win_attribs.height, 0, 0);
	XO_FLAG_ON(cache_update_query);

	XO_FLAG_OFF(cache_updating);

	return 1;
}
