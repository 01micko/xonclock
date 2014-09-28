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
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xrender.h>
#include <X11/Xft/Xft.h>

#include "image.h"
#include "xonclock.h"

/* looking for a file extension to obtain a file type */
int get_image_type_from_ext(char *path)
{
	char *ext = NULL;

	if (!path || path[0] == 0x0) {
		msg("NULL pointer passed");
		return IFMT_UNK;
	}

	ext = rindex(path, '.');
	if (!ext) {
		return IFMT_UNK;
	}

	if (*(ext + 1) == 0x0) {
		return IFMT_UNK;
	}

	if (strcasecmp(ext, ".png") == 0) {
		return IFMT_PNG;
	} else if (strcasecmp(ext, ".tif") == 0) {
		return IFMT_TIF;
	} else if (strcasecmp(ext, ".tiff") == 0) {
		return IFMT_TIF;
	} else if (strcasecmp(ext, ".jpg") == 0) {
		return IFMT_JPG;
	} else if (strcasecmp(ext, ".jpeg") == 0) {
		return IFMT_JPG;
	} else if (strcasecmp(ext, ".xpm") == 0) {
		return IFMT_XPM;
	}

	return IFMT_UNK;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	mask_from_alpha
 * $Desc:	creates a mask pixmap from alpha array
 * $Args:	(i) dpy, win, vis - gets info to create
 *		an appropriate pixmap
 *		(i) alpha - ALPHA array
 *		(i) width, height - size of a picture
 *		(i) threshold - alpha threshold value
 *		(o) pixmap - result pixmap
 * $Ret:	1 if OK, otherwise - 0
 * $Comments:	delete created pixmap after using
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int mask_from_alpha(Display *dpy,
		    Window *win,
		    Visual *vis,
		    unsigned int depth,
		    unsigned char * alpha,
		    unsigned int width,
		    unsigned int height,
		    unsigned int threshold,
		    Pixmap *pixmap)
{
	int scr = 0;
	unsigned char *pixmap_data = NULL;
	Pixmap _pixmap;
	XImage *image = NULL;
	unsigned long ipos = 0;
	unsigned int i, j;

	GC gc;
	XGCValues values;
	unsigned long bp, fp;

	/* check entry */
	if (!dpy || !win || !vis ||
	    !alpha || !pixmap) {
		msg("Bad argument passed");
		return 0;
	} else if (width < MIN_CLOCK_WIDTH ||
		   height < MIN_CLOCK_HEIGHT ||
		   width > MAX_CLOCK_WIDTH ||
		   height > MAX_CLOCK_HEIGHT) {
		msg("Bad geomerty size(s)");
		return 0;
	}

	scr = DefaultScreen(dpy);

	fp = BlackPixel(dpy, DefaultScreen(dpy));
	bp = WhitePixel(dpy, DefaultScreen(dpy));

	/* make a pixmap to be filled */
	_pixmap = XCreatePixmap(dpy, *win, width, height, 1);

	/* make an image */
	switch(depth) {
	case 32:
	case 24:
		pixmap_data = calloc(1, sizeof(char) * width * height * 4);
		break;
	case 16:
	case 15:
		pixmap_data = calloc(1, sizeof(char) * width * height * 2);
		break;
	case 8:
		pixmap_data = calloc(1, sizeof(char) * width * height);
		break;
	default:
		msg("Unsupported depth gained");
		XFreePixmap(dpy, _pixmap);
		return 0;
		break;
	}
	if (!pixmap_data) {
		msg("No free memory");
		XFreePixmap(dpy, _pixmap);
		return 0;
	}
	image = XCreateImage(dpy, vis,
			     1, ZPixmap, 0,
			     pixmap_data, width, height,
			     8, 0);
	if (!image) {
		msg("Unable to create image");
		XFreePixmap(dpy, _pixmap);
		free(pixmap_data);
		return 0;
	}

	values.foreground = 1;
	values.background = 0;
	gc = XCreateGC(dpy, _pixmap,
		       GCForeground | GCBackground,
		       &values);

	/* fill the image */
	for (j = 0, ipos = 0; j < height; j++) {
		for (i = 0; i < width; i++) {
			XPutPixel(image, i, j,
				  (alpha[ipos++] > threshold) ? bp : fp );
		}
	}

	/* copy results into our new pixmap */
	XPutImage(dpy, _pixmap, gc, image,
		  0, 0, 0, 0, width, height);
	*pixmap = _pixmap; /* get results back */

	/* ok, all work is done and we just need to release resources */
	XFreeGC(dpy, gc);
	free(pixmap_data);
	image->data = NULL;
	XDestroyImage(image);

	return 1;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	pixmap_from_rgb
 * $Desc:	creates pixmap from RGB array
 * $Args:	(i) dpy, win, gc, visual, depth - gets info to create
 *		an appropriate pixmap
 *		(i) rgb - RGB array
 *		(i) width, height - size of a picture
 *		(o) pixmap - result pixmap
 * $Ret:	1 if OK, otherwise - 0
 * $Comments:	delete created pixmap after using
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int pixmap_from_rgb(Display *dpy,
		    Window *win,
		    GC *gc,
		    Visual *visual,
		    unsigned int depth,
		    unsigned char * rgb,
		    unsigned int width,
		    unsigned int height,
		    Pixmap *pixmap)
{
	int scr = 0;
	unsigned char *pixmap_data = NULL;
	Pixmap _pixmap;
	XVisualInfo v_template;
	XVisualInfo *visual_info = NULL;
	XImage *image = NULL;
	int entries;
	unsigned long pixel;
	unsigned long ipos = 0;
	unsigned int i, j;

	unsigned char red_ls, red_rs;
	unsigned char green_ls, green_rs;
	unsigned char blue_ls, blue_rs;
	unsigned long red, green, blue;

	/* check entry */
	if (!dpy || !win || !gc || !visual || !depth ||
	    !rgb || !pixmap) {
		msg("Bad argument passed");
		return 0;
	} else if ( width < MIN_CLOCK_WIDTH ||
		    height < MIN_CLOCK_HEIGHT ||
		    width > MAX_CLOCK_WIDTH ||
		    height > MAX_CLOCK_HEIGHT) {
		msg("Bad geomerty size(s)");
		return 0;
	}

	scr = DefaultScreen(dpy);

	/* make pixmap to be filled */
	_pixmap = XCreatePixmap(dpy, (Drawable)*win,
				width, height, (unsigned int)depth);
	switch(depth) {
	case 32:
	case 24:
		pixmap_data = calloc(1, sizeof(char) * width * height * 4);
		break;
	case 16:
	case 15:
		pixmap_data = calloc(1, sizeof(char) * width * height * 2);
		break;
	case 8:
		pixmap_data = calloc(1, sizeof(char) * width * height);
		break;
	default:
		msg("Unsupported depth gained");
		XFreePixmap(dpy, _pixmap);
		return 0;
		break;
	}
	if (!pixmap_data) {
		msg("No free memory");
		XFreePixmap(dpy, _pixmap);
		return 0;
	}

	/* make new image to fill */
	image = XCreateImage(dpy, visual,
			     (unsigned int)depth, ZPixmap, 0,
			     pixmap_data, width, height,
			     8, 0);
	if (!image) {
		msg("Unable to create image");
		XFreePixmap(dpy, _pixmap);
		free(pixmap_data);
		return 0;
	}

	/* some info to get color shifts */
	v_template.visualid = XVisualIDFromVisual(visual);
	visual_info = XGetVisualInfo(dpy,
				     VisualIDMask,
				     &v_template,
				     &entries);
	switch (visual_info->class) {
	case TrueColor:
	case DirectColor:
		/* get masks */
		color_shift(visual_info->red_mask, &red_ls, &red_rs);
		color_shift(visual_info->green_mask, &green_ls, &green_rs);
		color_shift(visual_info->blue_mask, &blue_ls, &blue_rs);
		/* fill the image */
		for (j = 0, ipos = 0; j < height; j++) {
			for (i = 0; i < width; i++) {
				red = (unsigned long)rgb[ipos++] >> red_rs;
				green = (unsigned long)rgb[ipos++] >> green_rs;
				blue = (unsigned long)rgb[ipos++] >> blue_rs;
				pixel = (((red << red_ls) &
					  visual_info->red_mask)
					 | ((green << green_ls)
					    & visual_info->green_mask)
					 | ((blue << blue_ls)
					    & visual_info->blue_mask));
				XPutPixel(image, i, j, pixel);
			}
		}
		break;
	default:
		msg("The program may work with "
		    "TrueColor or DirectColor only");
		XFree(visual_info);
		XFreePixmap(dpy, _pixmap);
		free(pixmap_data);
		return 0;
	}

	/* copy results into our new pixmap */
	XPutImage(dpy, _pixmap, *gc, image,
		  0, 0, 0, 0, width, height);
	*pixmap = _pixmap; /* get results back */

	/* ok, all work is done and we just need to release resources */
	XFree(visual_info);
	free(pixmap_data);
	image->data = NULL;
	XDestroyImage(image);

	return 1;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	color_shift
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void color_shift(unsigned long mask,
		 unsigned char *left_shift,
		 unsigned char *right_shift)
{
	if (!left_shift || !right_shift)
		return;

	*left_shift = 0;
	*right_shift = 8;	/* to fit 8-bit value */

	if (mask != 0) {
		while ((mask & 0x1) == 0x0) {
			(*left_shift)++;
			mask >>= 0x1;
		}
		while ((mask & 0x1) == 0x1) {
			(*right_shift)--;
			mask >>= 0x1;
		}
	}
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	get_dpy_bg_rgb
 * $Desc:	returns RGB array of display root window background
 * $Args:	(i) dpy - display
 *		(i/o) rgb_prealloced - do not acllocate new memory for
 *		      results just use the supplied one; if NULL we
 *		      should allocate new memory
 *		(i) pos_x, pos_y, width, height - area of root
 *		window to get RGB data
 * $Ret:	pointer to allocated RGB array or NULL on error
 * $Comments:	calling routine must free RGB array
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
unsigned char * get_dpy_bg_rgb(Display *dpy, unsigned char *rgb_prealloced,
			       int pos_x, int pos_y,
			       unsigned int width, unsigned int height)
{
	unsigned char *rgb = NULL;
	int scr = 0;
	Visual * vis = NULL;
	XImage * image = NULL;

	/* check entry*/
	if (!dpy) {
		msg("Bad argument passed");
		return NULL;
	} else if (width < MIN_CLOCK_WIDTH ||
		   height < MIN_CLOCK_HEIGHT ||
		   width > MAX_CLOCK_WIDTH ||
		   height > MAX_CLOCK_HEIGHT) {
		msg("Bad geomerty size(s)");
		return NULL;
	}

	scr = DefaultScreen(dpy);
	vis = DefaultVisual(dpy, scr);

	image = XGetImage(dpy, DefaultRootWindow(dpy),
			  pos_x, pos_y, width, height,
			  AllPlanes, ZPixmap);
	if (!image) {
		msg("Unable to get image of the root window");
		return NULL;
	}

	/* here we can get NULL */
	rgb = get_rgb_from_ximage(dpy, vis, image, rgb_prealloced,
				  width, height);

	XDestroyImage(image);

	return rgb;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	merge_rgba_aa
 * $Desc:	merges two RGBA pictures and makes new composed
 *		alpha channel if needed
 * $Args:	(i) rgb_bg - background RGB
 *		(i) rgb_fg - foreground RGB
 *		(i) alfa_bg - background alpha channel (may be NULL)
 *		(i) alfa_fg - foreground alpha channel (may be NULL)
 *		(o) rgb_merged - merged RGB
 *		(o) alfa_merged - merged alpha channel (may be NULL)
 * $Ret:	1 if ok, otherwise - 0
 * $Comments:	1) see special cases in the code comments
 *		2) we assume that memory for output variables are already
 *		   allocated
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int merge_rgba_aa(unsigned char *rgb_bg,
		  unsigned char *rgb_fg,
		  unsigned char *alfa_bg,
		  unsigned char *alfa_fg,
		  unsigned char *rgb_merged,
		  unsigned char *alfa_merged,
		  int width, int height)
{
	unsigned int i;
	unsigned char *p, *p1, *p2;

	/* check entry */
	if (!rgb_bg || !rgb_fg || !rgb_merged) {
		msg("Bad argument passed");
		return 0;
	} else if (width < MIN_CLOCK_WIDTH ||
		   height < MIN_CLOCK_HEIGHT ||
		   width > MAX_CLOCK_WIDTH ||
		   height > MAX_CLOCK_HEIGHT) {
		msg("Bad geomerty size(s)");
		return 0;
	}

	/* check entry alphas */
	if (alfa_fg && alfa_bg && !alfa_merged) {
		msg("Bad argument passed");
		return 0;
	}

	/* if we got two alpha channels we use foreground channel
	 * to mess colors and then we compose a new alpha channel
	 * with more opacity values from both channels */
	if (alfa_fg && alfa_bg) {
		for (i = 0, p = alfa_fg; i < width * height * 3; i += 3) {
			rgb_merged[i+0] =
				COLOR_MESS(rgb_fg[i+0],
					   rgb_bg[i+0],
					   ALPHA_TO_DEC((*p)));
			rgb_merged[i+1] =
				COLOR_MESS(rgb_fg[i+1],
					   rgb_bg[i+1],
					   ALPHA_TO_DEC((*p)));
			rgb_merged[i+2] =
				COLOR_MESS(rgb_fg[i+2],
					   rgb_bg[i+2],
					   ALPHA_TO_DEC((*p)));
			p++;
		}
		/* mess the alpha channels */
		p1 = alfa_fg, p2 = alfa_bg, p = alfa_merged;
		for (i = 0; i < width * height; i++) {
			if (*p1 >= *p2) {
				*p = *p1;
			} else {
				*p = *p2;
			}
			p++, p1++, p2++;
		}
	} else if (alfa_fg) {
		/* if we got only foreground alpha channel we just
		 * mess the foreground and background colors */
		for (i = 0, p = alfa_fg; i < width * height * 3; i += 3) {
			rgb_merged[i+0] =
				COLOR_MESS(rgb_fg[i+0],
					   rgb_bg[i+0],
					   ALPHA_TO_DEC((*p)));
			rgb_merged[i+1] =
				COLOR_MESS(rgb_fg[i+1],
					   rgb_bg[i+1],
					   ALPHA_TO_DEC((*p)));
			rgb_merged[i+2] =
				COLOR_MESS(rgb_fg[i+2],
					   rgb_bg[i+2],
					   ALPHA_TO_DEC((*p)));
			p++;
		}
	} else {
		/* if we got only foreground RGB then just
		 * put foreground picture over the background */
		memcpy(rgb_merged, rgb_fg, width * height * 3);
	}

	return 1;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	get_rgb_from_image
 * $Desc:	returns RGB array of XImage data
 * $Args:	(i) dpy, vis, image - display, visual and image itself
 *		(i/o) rgb_prealloced - do not alloc new memory use supplied
 *		      one; if NULL we alloc new memory
 *		(i) width, height - image sizes
 * $Ret:	pointer to allocated RGB array or NULL on error
 * $Comments:	calling routine must free RGB array
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
unsigned char * get_rgb_from_ximage(Display *dpy, Visual *vis, XImage *image,
				    unsigned char *rgb_prealloced,
				    unsigned int width, unsigned int height)
{
	unsigned char *rgb = NULL;
	int scr = 0;
	XVisualInfo v_template;
	XVisualInfo *visual_info = NULL;
	int entries;
	unsigned long pixel;
	unsigned long ipos = 0;
	unsigned int i, j;

	unsigned char red_ls, red_rs;
	unsigned char green_ls, green_rs;
	unsigned char blue_ls, blue_rs;

	/* check entry */
	if (!dpy || !vis || !image) {
		msg("NULL pointer passed");
		return NULL;
	} else if ( width < MIN_CLOCK_WIDTH ||
		    height < MIN_CLOCK_HEIGHT ||
		    width > MAX_CLOCK_WIDTH ||
		    height > MAX_CLOCK_HEIGHT) {
		msg("Bad geomerty size(s)");
		return NULL;
	}

	scr = DefaultScreen(dpy);
	if (!rgb_prealloced)
		rgb = calloc(1, sizeof(char) * width * height * 3);
	else
		rgb = rgb_prealloced;
	if (!rgb) {
		msg("No free memory");
		goto err;
	}

	v_template.visualid = XVisualIDFromVisual(vis);
	visual_info = XGetVisualInfo(dpy, VisualIDMask,
				     &v_template, &entries);

	switch (visual_info->class) {
	case DirectColor:
	case TrueColor:
		/* get masks */
		color_shift(visual_info->red_mask, &red_ls, &red_rs);
		color_shift(visual_info->green_mask, &green_ls, &green_rs);
		color_shift(visual_info->blue_mask, &blue_ls, &blue_rs);
		/* fill out our RGB array */
		for (j = 0, ipos = 0; j < height; j++) {
			for (i = 0; i < width; i++) {
				pixel = XGetPixel(image, i, j);
				rgb[ipos++] = (unsigned char)
					(unsigned long)(
					(pixel & visual_info->red_mask) >>
					red_ls << red_rs);
				rgb[ipos++] = (unsigned char)
					(unsigned long)(
					(pixel & visual_info->green_mask) >>
					green_ls << green_rs);
				rgb[ipos++] = (unsigned char)
					(unsigned long)(
					(pixel & visual_info->blue_mask) >>
					blue_ls << blue_rs);
			}
		}
		break;
	default:
		msg("The program may work with "
		    "TrueColor or DirectColor only");
		goto err;
		break;
	} /* switch (visual_info->c_class) */

	XFree(visual_info);

	return rgb;

 err:
	if (visual_info)
		XFree(visual_info);
	if (rgb && !rgb_prealloced)
		free(rgb);
	return NULL;
}
