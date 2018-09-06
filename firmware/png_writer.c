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
#include "pattern.h"
#include "png_writer.h"

static const uint32_t lookup_colors[] = {
	MK_RGB(0x27, 0xae, 0x60),
	MK_RGB(0x34, 0x98, 0xdb),
	MK_RGB(0xe6, 0x7e, 0x22),
	MK_RGB(0xc0, 0x39, 0x2b),
};
#define PALETTE_SIZE 		(sizeof(lookup_colors) / sizeof(uint32_t))

typedef uint32_t (*color_lookup_fnc)(const struct pattern_t *color, uint8_t color_index);

struct png_write_file_ctx_t {
	FILE *f;
};

struct png_write_mem_ctx_t {
	bool success;
	struct membuf_t *membuf;
};

struct png_write_ctx_t {
	bool (*init_io_callback)(struct png_write_ctx_t *ctx, png_structp png_ptr);
	union {
		struct png_write_file_ctx_t file;
		struct png_write_mem_ctx_t mem;
	} custom;
};


static const struct png_write_options_t default_write_options = {
	.pixel_width = 4,
	.pixel_height = 4,
	.grid_width = 1,
	.grid_color = 0xaaaaaaaa,
	.color_scheme = COLSCHEME_PRETTY,
};

static uint32_t lookup_color_raw(const struct pattern_t *pattern, uint8_t color_index) {
	if (color_index == 0) {
		return PIXEL_FULLY_TRANSPARENT;
	} else {
		if ((color_index - 1) < PALETTE_SIZE) {
			/* Choose fancy colors for the first few */
			return lookup_colors[(color_index - 1) % PALETTE_SIZE];
		} else {
			/* Choose grayscale for the others */
			uint8_t gray_value = (color_index - 1) * (255 / pattern->used_colors);
			return MK_GRAY(gray_value);
		}
	}
}

static uint32_t lookup_color_pretty(const struct pattern_t *pattern, uint8_t color_index) {
	if (color_index == 0) {
		return PIXEL_FULLY_TRANSPARENT;
	} else {
		return lookup_colors[(color_index - 1) % PALETTE_SIZE];
	}
}

static color_lookup_fnc get_lookup_function(enum colorscheme_t colscheme) {
	switch (colscheme) {
		case COLSCHEME_PRETTY: return lookup_color_pretty;
		case COLSCHEME_RAW: return lookup_color_raw;
	}
	return lookup_color_raw;
}


static bool init_file_io(struct png_write_ctx_t *ctx, png_structp png_ptr) {
	png_init_io(png_ptr, ctx->custom.file.f);
	return true;
}

static void write_memory_callback(png_structp png_ptr, uint8_t *new_data, png_size_t length) {
	void *vctx = png_get_io_ptr(png_ptr);
	struct png_write_ctx_t *ctx = (struct png_write_ctx_t*)vctx;
	if (!ctx->custom.mem.success) {
		return;
	}
	if (!membuf_append(ctx->custom.mem.membuf, new_data, length)) {
		ctx->custom.mem.success = false;
	}
}

static void flush_memory_callback(png_structp png_ptr) {
}

static bool init_mem_io(struct png_write_ctx_t *ctx, png_structp png_ptr) {
	png_set_write_fn(png_ptr, ctx, write_memory_callback, flush_memory_callback);
	return true;
}

static bool png_write_pattern_generic(const struct pattern_t *pattern, struct png_write_ctx_t *ctx, const struct png_write_options_t *options) {
	if (options == NULL) {
		options = &default_write_options;
	}

	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) {
		perror("png_create_write_struct");
		return false;
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		perror("png_create_info_struct");
		png_destroy_write_struct(&png_ptr, NULL);
		return false;
	}

	if (!ctx->init_io_callback(ctx, png_ptr)) {
		perror("Failed to initialize I/O");
		png_destroy_info_struct(png_ptr, &info_ptr);
		png_destroy_write_struct(&png_ptr, NULL);
		return false;
	}

	int width = (pattern->width * options->pixel_width) + ((pattern->width - 1) * options->grid_width);
	int height = (pattern->height * options->pixel_height) + ((pattern->height - 1) * options->grid_width);
	png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	png_write_info(png_ptr, info_ptr);
	png_destroy_info_struct(png_ptr, &info_ptr);

	color_lookup_fnc color_lookup = get_lookup_function(options->color_scheme);
	for (int y = 0; y < pattern->height; y++) {
		uint32_t png_row[width];
		memset(png_row, 0, sizeof(png_row));

		const uint8_t *ptrn_row = pattern_row(pattern, y);

		/* Write rows of pixels */
		for (int rpty = 0; rpty < options->pixel_height; rpty++) {
			/* Write color pixels */
			for (int x = 0; x < pattern->width; x++) {
				uint8_t pixel = ptrn_row[x];
				uint32_t color = color_lookup(pattern, pixel);
				for (int rptx = 0; rptx < options->pixel_width; rptx++) {
					png_row[(x * options->pixel_width) + (x * options->grid_width) + rptx] = color;
				}
			}
			/* Write grid pixels */
			for (int x = 1; x < pattern->width; x++) {
				for (int rptx = 0; rptx < options->grid_width; rptx++) {
					png_row[(x * options->pixel_width) + ((x - 1) * options->grid_width) + rptx] = options->grid_color;
				}

			}
			png_write_row(png_ptr, (uint8_t*)png_row);
		}

		/* Write grid row */
		if (y != (pattern->height - 1)) {
			/* Write grid rows */
			for (int x = 0; x < width; x++) {
				png_row[x] = options->grid_color;
			}

			for (int rpty = 0; rpty < options->grid_width; rpty++) {
				png_write_row(png_ptr, (uint8_t*)png_row);
			}
		}
	}
	png_write_end(png_ptr, NULL);

	png_destroy_write_struct(&png_ptr, NULL);
	return true;
}

bool png_write_pattern(const struct pattern_t *pattern, const char *filename, const struct png_write_options_t *options) {
	struct png_write_ctx_t ctx = {
		.init_io_callback = init_file_io,
	};

	ctx.custom.file.f = fopen(filename, "w");
	if (!ctx.custom.file.f) {
		perror(filename);
		return false;
	}

	bool success = png_write_pattern_generic(pattern, &ctx, options);

	fclose(ctx.custom.file.f);
	return success;
}

bool png_write_pattern_mem(const struct pattern_t *pattern, struct membuf_t *membuf, const struct png_write_options_t *options) {
	struct png_write_ctx_t ctx = {
		.init_io_callback = init_mem_io,
		.custom.mem = {
			.membuf = membuf,
			.success = true,
		},
	};

	bool success = png_write_pattern_generic(pattern, &ctx, options);

	return success && ctx.custom.mem.success;
}
