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
#include <string.h>

#include <jpeglib.h>

#include "../xonclock.h"

#include "jpeg.h"


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	read_jpeg
 * $Desc:	read JPEG file
 * $Args:	(i) filename - file to be readed
 *		(o) width, height - size of the readed image
 *		(o) rgb - readed data (MEMORY WILL BE ALLOCATED)
 *		[o] a - alpha channel. It's not used by JPEG images
 *		and if set to not NULL then it will be filled with
 *		values of 255 and MEMORY WILL BE ALLOCATED
 * $Ret:	1 on success, otherwise - 0
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int
read_jpeg(const char * filename,
	  int * width,
	  int * height,
	  unsigned char ** rgb,
	  unsigned char ** alpha)
{
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	unsigned char *ptr = NULL;
	unsigned int i, ipos;
	FILE *infile;

	/* check entry */
	if (!filename || !width || !height || !rgb) {
		msg("NULL pointer passed");
		return 0;
	}

	infile = fopen(filename, "rb");
	if (!infile) {
		msg("Unable to open file: %s", filename);
		return 0;
	}

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, infile);
	jpeg_read_header(&cinfo, TRUE);
	jpeg_start_decompress(&cinfo);

	*width = cinfo.output_width;
	*height = cinfo.output_height;

	/* check sizes */
	if (*width < MIN_CLOCK_WIDTH || *width > MAX_CLOCK_WIDTH ||
	    *height < MIN_CLOCK_HEIGHT || *height > MAX_CLOCK_HEIGHT) {
		msg("Incorrect image sizes");
		fclose(infile);
		return 0;
	}

	rgb[0] = malloc(3 * cinfo.output_width * cinfo.output_height);
	if (!rgb[0]) {
		msg("No free memory");
		fclose(infile);
		return 0;
	}

	if (alpha) {
		alpha[0] = malloc(cinfo.output_width * cinfo.output_height);
		if (!alpha[0]) {
			free(rgb[0]);
			msg("No free memory");
			fclose(infile);
			return 0;
		}
		memset(alpha[0], 255,
		       cinfo.output_width * cinfo.output_height);
	}

	if (cinfo.output_components == 3) {
		ptr = rgb[0];
		while (cinfo.output_scanline < cinfo.output_height) {
			jpeg_read_scanlines(&cinfo, &ptr, 1);
			ptr += 3 * cinfo.output_width;
		}
	} else if (cinfo.output_components == 1) {
		ptr = malloc(cinfo.output_width);
		if (ptr == NULL) {
			free(rgb[0]);
			if (alpha) {
				free(alpha[0]);
			}
			fclose(infile);
			msg("No free memory");
			return 0;
		}

		ipos = 0;
		while (cinfo.output_scanline < cinfo.output_height) {
			jpeg_read_scanlines(&cinfo, &ptr, 1);
			for (i = 0; i < cinfo.output_width; i++) {
				memset(rgb[0] + ipos, ptr[i], 3);
				ipos += 3;
			}
		}
		free(ptr);
	}


	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	fclose(infile);

	return 1;
}
