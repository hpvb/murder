/* This file is part of remuder
 *
 * Copyright (C) 2014  Hein-Pieter van Braam <hp@tmm.cx>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <stdio.h>
#include <string.h>

#include "byterun1.h"

/*
 * Byterun1 decoder
 * From http://wiki.amigaos.net/index.php/ILBM_IFF_Interleaved_Bitmap
 *
 * LOOP until produced the desired number of bytes
 *	Read the next source byte into n
 *	SELECT n FROM
 *		[0..127]	=> copy the next n+1 bytes literally
 *		[-1..-127]	=> replicate the next byte -n+1 times
 *		-128	=> noop
 *		ENDCASE;
 *	ENDLOOP;
 */

void byterun1_decode(int toread, char *in, char *out, int *consumed,
		int *produced) {
	int i;
	char byte;

	*produced = 0;
	*consumed = 0;

	while (*produced < toread) {
		byte = *(in + *consumed);
		if (byte >= 0) {
			//printf("Copying %i bytes\n", byte + 1);
			memcpy(out + *produced, in + *consumed + 1, byte + 1);
			*consumed += byte + 2;
		}

		if (byte < 0) {
			byte *= -1;
			//printf("Duplicating %i bytes\n", byte + 1);
			memset(out + *produced, *(in + *consumed + 1), byte + 1);
			*consumed += 2;
		}

		if (byte == -128)
			printf("Noop\n");

		*produced += byte + 1;
	}

	if (*produced > toread) {
		printf("+%i ", *produced - toread);
		for (i = 0; i < *produced; ++i) {
			//printf("0x%02X ", (unsigned char)in[i]);
		}
		printf("\n");
	}
}
