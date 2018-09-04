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
#include <string.h>
#include <stdlib.h>
#include "tokenizer.h"

struct tokens_t* tok_create(const char *buffer) {
	struct tokens_t *tokens = calloc(1, sizeof(struct tokens_t));
	if (!tokens) {
		return NULL;
	}
	tokens->mutable_copy = strdup(buffer);
	if (!tokens->mutable_copy) {
		free(tokens);
		return NULL;
	}

	char *saveptr = NULL;
	char *next_token = strtok_r(tokens->mutable_copy, " ", &saveptr);
	while ((next_token) && (tokens->token_cnt < MAX_TOKEN_CNT)) {
		tokens->token[tokens->token_cnt].string = next_token;
		tokens->token_cnt++;
		next_token = strtok_r(NULL, " ", &saveptr);
	}
	return tokens;
}

void tok_free(struct tokens_t *tokens) {
	if (tokens) {
		free(tokens->mutable_copy);
		free(tokens);
	}
}
