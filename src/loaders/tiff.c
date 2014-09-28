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
#include <tiffio.h>

#include "../xonclock.h"

#include "tiff.h"


/* just read TIFF file to the RGB and ALPHA array
 * which must be freed after by caller */
int
read_tiff(char * path,
	  unsigned int * width,
	  unsigned int * height,
	  unsigned char ** rgb,
	  unsigned char ** alpha)
{
	TIFF * tif = NULL;
	uint32  _w, _h;
	uint32 * raster = NULL;
	unsigned char * _rgb = NULL, * _alpha = NULL;
	size_t npixels;
	unsigned int j, k;
	long i;

	/* check entry */
	if (!path || !width || !height || !rgb) {
		msg("NULL pointer passed");
		return 0;
	}

	tif = TIFFOpen(path, "r");
	if (!tif) {
		msg("Unable to open: %s", path);
		return 0;
	}

	TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &_w);
	TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &_h);

	/* check sizes */
	if (_w < MIN_CLOCK_WIDTH || _w > MAX_CLOCK_WIDTH ||
	    _h < MIN_CLOCK_HEIGHT || _h > MAX_CLOCK_HEIGHT) {
		msg("Incorrect image sizes");
		TIFFClose(tif);
		return 0;
	}

	npixels = _w * _h;

	/* get the memory we need */
	raster = (uint32*) _TIFFmalloc(npixels * sizeof(uint32));
	_rgb = calloc(3, sizeof(char) * _w * _h);
	_alpha = calloc(1, sizeof(char) * _w * _h);
	if (!raster || ! _rgb || !_alpha) {
		msg("No free memory");
		goto err;
	}

	if (TIFFReadRGBAImageOriented(tif, _w, _h, raster,
				      ORIENTATION_TOPLEFT, 0)) {
		for (i = 0, j = 0, k = 0; i < _w * _h; i++) {
			_rgb[j++] = (raster[i] & 0x000000ff) >> 0;
			_rgb[j++] = (raster[i] & 0x0000ff00) >> 8;
			_rgb[j++] = (raster[i] & 0x00ff0000) >> 16;
			_alpha[k++] = (raster[i] & 0xff000000) >> 24;
		}
	}
	_TIFFfree(raster);
	TIFFClose(tif);

	/* set results */
	*rgb = _rgb;
	*alpha = _alpha;
	*width = _w;
	*height = _h;

	return 1;

 err:
	if (raster)
		_TIFFfree(raster);
	if (tif)
		TIFFClose(tif);
	if (_rgb)
		free(_rgb);
	if (_alpha)
		free(_alpha);
	return 0;
}
