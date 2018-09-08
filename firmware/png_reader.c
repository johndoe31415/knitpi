/*
 *	knitpi - Raspberry Pi interface for Brother KH-930 knitting machine
 *	Copyright (C) 2018-2018 Johannes Bauer
 *
 *	This file is part of knitpi.
 *
 *	knitpi is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; this program is ONLY licensed under
 *	version 3 of the License, later versions are explicitly excluded.
 *
 *	knitpi is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with knitpi; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *	Johannes Bauer <JohannesBauer@gmx.de>
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <string.h>
#include <libpng16/png.h>
#include "png_reader.h"
#include "logging.h"

static void membuf_read_data_fn(png_structp png_ptr, uint8_t *data, png_size_t length) {
	void *vmembuf = png_get_io_ptr(png_ptr);
	struct membuf_t *membuf = (struct membuf_t*)vmembuf;
	membuf_read(membuf, data, length);
}


static const char* png_color_type_to_str(uint8_t color_type) {
	switch (color_type) {
		case PNG_COLOR_TYPE_GRAY: return "GRAY";
		case PNG_COLOR_TYPE_PALETTE: return "PALETTE";
		case PNG_COLOR_TYPE_GRAY_ALPHA: return "GRAY_ALPHA";
		case PNG_COLOR_TYPE_RGB: return "RGB";
		case PNG_COLOR_TYPE_RGBA: return "RGB_ALPHA";
	}
	return "Unknown";
}

struct pattern_t* png_read_pattern(struct membuf_t *membuf, unsigned int offsetx, unsigned int offsety) {
	struct pattern_t *pattern = NULL;

	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) {
		perror("png_create_read_struct");
		return NULL;
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		perror("png_create_info_struct");
		return NULL;
	}

	uint32_t *pixel_data = NULL;
	if (setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		free(pixel_data);
		return pattern;
	}

	membuf_rewind(membuf);
	png_set_read_fn(png_ptr, membuf, membuf_read_data_fn);
	png_read_info(png_ptr, info_ptr);

	uint32_t width = png_get_image_width(png_ptr, info_ptr);
	uint32_t height = png_get_image_height(png_ptr, info_ptr);
	uint8_t color_type = png_get_color_type(png_ptr, info_ptr);
	logmsg(LLVL_DEBUG, "PNG decoded: %u x %u pixels", width, height);

	pattern = pattern_new(width + offsetx, height + offsety);
	if (!pattern) {
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		return NULL;
	}

	if (png_get_bit_depth(png_ptr, info_ptr) == 16) {
		logmsg(LLVL_TRACE, "PNG stripped from 16 bits to 8.");
		png_set_strip_16(png_ptr);
	}

	if (color_type == PNG_COLOR_TYPE_PALETTE) {
		logmsg(LLVL_TRACE, "PNG palette converted to RGB.");
		png_set_palette_to_rgb(png_ptr);
	} else if ((color_type == PNG_COLOR_TYPE_GRAY) && (png_get_bit_depth(png_ptr, info_ptr) < 8)) {
		logmsg(LLVL_TRACE, "PNG grayscale color depth converted to 8 bit.");
		png_set_expand_gray_1_2_4_to_8(png_ptr);
	}

	if ((color_type == PNG_COLOR_TYPE_GRAY) || (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)) {
		logmsg(LLVL_TRACE, "PNG converting grayscale to RGB.");
		png_set_gray_to_rgb(png_ptr);
	}

	if ((color_type == PNG_COLOR_TYPE_RGB) || (color_type == PNG_COLOR_TYPE_GRAY) || (color_type == PNG_COLOR_TYPE_PALETTE)) {
		logmsg(LLVL_TRACE, "PNG adding alpha channel.");
		png_set_add_alpha(png_ptr, 0xff, PNG_FILLER_AFTER);
	}

	png_read_update_info(png_ptr, info_ptr);
	color_type = png_get_color_type(png_ptr, info_ptr);

	if ((color_type != PNG_COLOR_TYPE_RGBA) || (png_get_bit_depth(png_ptr, info_ptr) != 8)) {
		logmsg(LLVL_WARN, "Resulting PNG image not RGBA8: color_type = %s (0x%x), bit_depth = %d", png_color_type_to_str(color_type), color_type, png_get_bit_depth(png_ptr, info_ptr));
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		pattern_free(pattern);
		return NULL;
	}

	pixel_data = malloc(width * height * 4);
	if (!pixel_data) {
		logmsg(LLVL_ERROR, "Failed to allocate %d bytes for pixel data of PNG image.", width * height * 4);
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		pattern_free(pattern);
		return NULL;
	}

	uint32_t *row_pointers[height];
	for (int y = 0; y < height; y++) {
		row_pointers[y] = pixel_data + (y * width);
	}
	png_read_image(png_ptr, (uint8_t**)row_pointers);

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			uint32_t pixel_rgba = row_pointers[y][x];
			pattern_set_rgba(pattern, x + offsetx, y + offsety, pixel_rgba);
		}
	}

	free(pixel_data);

	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	return pattern;
}

