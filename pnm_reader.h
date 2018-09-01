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

#ifndef __PNM_READER_H__
#define __PNM_READER_H__

struct pnmfile_t {
	unsigned int width, height;
	uint8_t *image_data;
};

/*************** AUTO GENERATED SECTION FOLLOWS ***************/
struct pnmfile_t* pnmfile_read(const char *filename);
const uint8_t* pnmfile_row(const struct pnmfile_t *pnmfile, unsigned int y);
void pnmfile_dump(const struct pnmfile_t *pnmfile);
void pnmfile_free(struct pnmfile_t *pnmfile);
/***************  AUTO GENERATED SECTION ENDS   ***************/

#endif
