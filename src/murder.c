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
#include <stdlib.h>
#include <SDL/SDL.h>

#include "types.h"
#include "display.h"
#include "byterun1.h"

/*
 * Images are interlaced. Images are stored in groups of 4 subimages per chunk,
 * 4 chunks per image. If an image width isn't divisible by 4 each the first N
 * chunks have wider subimages. N is determined by width % 4.
 *
 * An image can be reconstructed by choosing subsequent pixels from each
 * chunk. Taking care to select extra pixels from the wider chunks at the end
 * of each scanline.
 */

void deinterlace(int out_width, int out_height, char *chunks[4], char *out) {
	int x, y;
	char *chunkp[4];
	memcpy(chunkp, chunks, sizeof(chunkp));

	for (y = 0; y < out_height; ++y) {
		for (x = 0; x < out_width; ++x) {
			*out++ = *(chunkp[x % 4]++);
		}
	}
}

/* Every compressed chunk has a data header of N bytes. N is the fist 2 bytes
 * as an LE 16bit value. After this the chunk is compressed with byterun1.
 *
 * When an image has an height that can't be divided by four, extra pixels of
 * N subimages exist at the end of the regular-sized scanlines. N is determined
 * by height % 4.
 */

void decompress_chunk(int width, int height, int extra_bytes, char *chunk,
		char *out) {
	int consumed, produced;
	char *outp, *inp;
	unsigned short data_size = 0, row;

	data_size = *((short *) chunk);
	printf("Unknown data size : %i\n", data_size);

	inp = chunk + data_size;
	outp = out;

	for (row = 0; row < height; ++row) {
		byterun1_decode(width, inp, outp, &consumed, &produced);

		outp += produced;
		inp += consumed;
	}

	if (extra_bytes > 0) {
		byterun1_decode(extra_bytes, inp, outp, &consumed, &produced);
	}
}

/* An image is optionally compressed. When decompressing it is necessary to
 * know up-front how many bytes each scanline will be. The algorithm is
 * explained at deinterlace()
 */

void parse_image(image * image, char *out) {
	int i, chunk_width, chunk_height, extra_bytes = 0;
	char *chunk[4];

	if (image->maybe_compressed) {
		printf("Compressed image\n");

		for (i = 0; i < 4; ++i) {
			chunk_width = image->width;
			chunk_height = image->height / 4;

			if (image->width % 4 > 0) {
				if (i < chunk_width % 4) {
					chunk_width += 2;
				} else {
					chunk_width -= 2;
				}
			}

			extra_bytes = (image->height % 4) * (chunk_width / 4);

			chunk[i] = malloc(chunk_width * chunk_height + extra_bytes);

			decompress_chunk(chunk_width, chunk_height, extra_bytes,
					image->chunk[i], chunk[i]);
		}

		deinterlace(image->width, image->height, chunk, out);

		for (i = 0; i < 4; ++i) {
			free(chunk[i]);
		}

	} else {
		printf("Uncompressed image\n");
		deinterlace(image->width, image->height, image->chunk, out);
	}
}

void dump_cursor(FILE * file) {
	printf("Assuming there are 3 cursors\n");
	char first_chunk[28];
	char second_chunk[64];
	fread(first_chunk, 28, 1, file);
	fread(second_chunk, 32, 2, file);
}

/* Palettes are stored per color component. Each color component is stored
 * in turn.
 */

void load_palette(FILE * file) {
	int i;
	palette palette;

	printf("Loading header\n");
	fread(&palette, sizeof(palette), 1, file);
	for (i = 0; i < sizeof(palette) / sizeof(short); ++i) {
		printf("%i ", *(((unsigned short *) (&palette)) + i));
	}
	printf("\n");

	printf("Palette has %i colors\n", palette.ncolors);

	printf("Loading palette\n");
	char r[256], g[256], b[256];
	fread(r, 256, 1, file);
	fread(g, 256, 1, file);
	fread(b, 256, 1, file);

	/* 6-bit palette, convert to 8 bit */
	for (i = 0; i < NCOLORS; ++i) {
		colors[i].r = (r[i] << 2) | (r[i] >> 4);
		colors[i].g = (g[i] << 2) | (g[i] >> 4);
		colors[i].b = (b[i] << 2) | (b[i] >> 4);
	}
}

void dump_image(FILE * file) {
	int i;

	printf("Dumping header\n");
	image image;

	fread(&image, 46, 1, file);
	for (i = 0; i < 46 / sizeof(short); ++i) {
		printf("%i ", *(((unsigned short *) (&image)) + i));
	}

	printf("Image dimensions: %ix%i\n", image.width, image.height);
	printf("Image compressed flag set to 0x%04X\n", image.maybe_compressed);

	short size;

	for (i = 0; i < 4; ++i) {
		size = image.chunk_sizes[i];
		image.chunk[i] = malloc(size);
		fread(image.chunk[i], size, 1, file);
	}

	FILE *dump_file = fopen("out.dump", "wb+");
	for (i = 0; i < 4; ++i) {
		fwrite(image.chunk[i], image.chunk_sizes[i], 1, dump_file);
	}
	fclose(dump_file);

	char *buffer;
	buffer = malloc(image.width * image.height);

	parse_image(&image, buffer);

	dump_file = fopen("out3.dump", "wb+");
	fwrite(buffer, image.width * image.height, 1, dump_file);
	fclose(dump_file);

	printf("Displaying %i bytes\n", image.width * image.height);
	display_buffer(image.width, image.height, buffer);

	free(buffer);

	for (i = 0; i < 4; ++i)
		free(image.chunk[i]);
}

void dump_unk1(FILE * file) {
	int i;

	printf("Dumping header\n");
	unsigned short header[15];

	fread(header, 30, 1, file);
	for (i = 0; i < 15; ++i) {
		printf("%i ", header[i]);
	}
	printf("\n");

	char *buffer;
	buffer = malloc(header[11]);
	fread(buffer, header[11], 1, file);

	FILE *dump_file = fopen("out.dump", "wb+");
	fwrite(buffer, header[11], 1, dump_file);
	fclose(dump_file);
}

void dump_audio(FILE * file) {
	int i;

	printf("Dumping header\n");
	unsigned short header[35];

	fread(header, 70, 1, file);
	for (i = 0; i < 35; ++i) {
		printf("%i ", header[i]);
	}
	printf("\n");

	char *buffer;
	buffer = malloc(header[8]);
	fread(buffer, header[8], 1, file);

	FILE *dump_file = fopen("out.dump", "wb+");
	fwrite(buffer, header[8], 1, dump_file);
	fclose(dump_file);
}

int main(int argc, char *argv[]) {
	init_sdl();

	FILE *file = fopen(argv[1], "rb");
	if (!file) {
		printf("File %s not found\n", argv[1]);
		return 1;
	}

	while (1) {
		short type;
		if (fread(&type, 2, 1, file) <= 0)
			break;
		fseek(file, -2, SEEK_CUR);

		switch (type) {
		case DAT_TYPE_CURSOR:
			printf("Found cursor: 0x%04X\n", type);
			dump_cursor(file);
			break;
		case DAT_TYPE_PALETTE:
			printf("Found palette: 0x%04X\n", type);
			load_palette(file);
			break;
		case DAT_TYPE_IMAGE:
			printf("Found image: 0x%04X\n", type);
			dump_image(file);
			break;
		case RTF_TYPE_AUDIO:
			printf("Found audio: 0x%04X\n", type);
			dump_audio(file);
			break;
		case RTF_TYPE_UNK1:
			printf("Found unk1: 0x%04X\n", type);
			dump_unk1(file);
			break;
		default:
			printf("Unknown type 0x%04X\n", type);
			return 1;
			break;
		}
	}
	fclose(file);

	return 0;
}
