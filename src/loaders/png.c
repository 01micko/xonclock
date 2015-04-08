/* -*- mode: linux-c -*-
 *
 *      Copyright (C) 2006 Cyrill Gorcunov <gclion@mail.ru>
 *
 *	Based on:
 *	Copyright (C) 2002 Hari Nair <hari@alumni.caltech.edu>
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

#include <stdio.h>
#include <stdlib.h>

#include <png.h>

#include "../xonclock.h"

#include "png.h"


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	read_png
 * $Desc:	read PNG file
 * $Args:	(i) filename - file to be readed
 *		(o) width, height - size of the readed image
 *		(o) rgb - readed data (MEMORY WILL BE ALLOCATED)
 *		(o) alpha - readed alpha channel (MEMORY WILL BE
 *		ALLOCATED) if there is no alpha channels then set
 *		it to NULL
 * $Ret:	1 on success, otherwise - 0
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int
read_png(const char * filename,
	 int * width, int * height,
	 unsigned char ** rgb,
	 unsigned char ** alpha)
{
	FILE * infile = NULL;

	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	png_bytepp row_pointers = NULL;

	unsigned char *ptr = NULL;
	png_uint_32 w, h;
	int bit_depth, color_type, interlace_type;
	int i = 0, j = 0, ipos = 0;
	int rc = 0;
	char buf[4];
	unsigned char *ubuf = (unsigned char *) buf;

	unsigned char * _rgb = NULL;
	unsigned char * _alpha = NULL;

	/* check entry */
	if (!filename	||
	    !width	||
	    !height	||
	    !rgb	||
	    !alpha) {
		msg("Bad argument passed");
		return 0;
	}

	*rgb = NULL;
	*alpha = NULL;

	/* check for PNG image */
	infile = fopen(filename, "rb");
	if (!infile) {
		msg("Unable to open %s", filename);
		return 0;
	}
	fread(buf, 1, 4, infile);
	fclose(infile);
	if ((ubuf[0] != 0x89) || strncmp("PNG", buf+1, 3) != 0) {
		msg("The file %s has invalid PNG struct", filename);
		return 0;
	}

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
					 (png_voidp) NULL,
					 (png_error_ptr) NULL,
					 (png_error_ptr) NULL);
	if (!png_ptr) {
		msg("libpng error catched");
		rc = 0;
		goto err;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		msg("libpng error catched");
		rc = 0;
		goto err;
	}
	
	if (setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_read_struct(&png_ptr, &info_ptr,
					(png_infopp) NULL);
		msg("libpng error catched");
		rc = 0;
		goto err;
	}

	infile = fopen(filename, "rb");
	if (!infile) {
		msg("Unable to open file: %s", filename);
		rc = 0;
		goto err;
	}

	png_init_io(png_ptr, infile);
	png_read_info(png_ptr, info_ptr);

	png_get_IHDR(png_ptr, info_ptr, &w, &h, &bit_depth, &color_type,
		     &interlace_type, (int *) NULL, (int *) NULL);

	/* check sizes */
	if (w < MIN_CLOCK_WIDTH || w > MAX_CLOCK_WIDTH ||
	    h < MIN_CLOCK_HEIGHT || h > MAX_CLOCK_HEIGHT) {
		msg("Incorrect image sizes");
		rc = 0;
		goto err;
	}

	*width = (int) w;
	*height = (int) h;

	if (color_type == PNG_COLOR_TYPE_RGB_ALPHA ||
	    color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
		_alpha = malloc(*width * *height);
		if (_alpha == NULL) {
			msg("No free memory");
			rc = 0;
			goto err;
		}
	}

	/* Change a paletted/grayscale image to RGB */
	if (color_type == PNG_COLOR_TYPE_PALETTE && bit_depth <= 8)
		png_set_expand(png_ptr);

	/* Change a grayscale image to RGB */
	if (color_type == PNG_COLOR_TYPE_GRAY ||
	    color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png_ptr);

	/* If the PNG file has 16 bits per channel, strip them down to 8 */
	if (bit_depth == 16)
		png_set_strip_16(png_ptr);

	/* use 1 byte per pixel */
	png_set_packing(png_ptr);

	row_pointers = malloc(*height * sizeof(png_bytep));
	if (row_pointers == NULL) {
		msg("No free memory");
		rc = 0;
		goto err;
	}

	for (i = 0, ipos = 0; i < *height; i++, ipos++) {
		row_pointers[i] = malloc(4 * *width);
		if (row_pointers == NULL) {
			msg("No free memory");
			rc = 0;
			goto err;
		}
	}

	png_read_image(png_ptr, row_pointers);

	_rgb = malloc(3 * *width * *height);
	if (_rgb == NULL) {
		msg("No free memory");
		rc = 0;
		goto err;
	}

	if (_alpha == NULL) {
		ptr = _rgb;
		for (i = 0; i < *height; i++) {
			memcpy(ptr, row_pointers[i], 3 * *width);
			ptr += 3 * *width;
		}
	} else {
		ptr = _rgb;
		for (i = 0; i < *height; i++) {
			for (j = 0, ipos = 0; j < *width; j++) {
				*ptr++ = row_pointers[i][ipos++];
				*ptr++ = row_pointers[i][ipos++];
				*ptr++ = row_pointers[i][ipos++];
				_alpha[i * *width + j] =
					row_pointers[i][ipos++];
			}
		}
	}

	png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);

	for (i = 0; i < *height; i++)
		free(row_pointers[i]);
	free(row_pointers);
	fclose(infile);

	*alpha = _alpha;
	*rgb = _rgb;

	return 1;

 err:
	if (infile)
		fclose(infile);
	if (_rgb)
		free(_rgb);
	if (_alpha)
		free(_alpha);
	if (row_pointers) {
		if (row_pointers[0]) {
			for (i = 0; i < ipos; i++) {
				free(row_pointers[i]);
			}
		}
		free(row_pointers);
	}
	if (info_ptr)
		png_destroy_read_struct(&png_ptr, &info_ptr,
					(png_infopp) NULL);
	else
		if (png_ptr)
			png_destroy_read_struct(&png_ptr, (png_infopp) NULL,
						(png_infopp) NULL);
	return rc;
}
