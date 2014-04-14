/*
 * types.h
 *
 *  Created on: Apr 10, 2014
 *      Author: hp
 */

#ifndef TYPES_H_
#define TYPES_H_

#define DAT_TYPE_CURSOR  0x0AD4
#define DAT_TYPE_PALETTE 0x09E4
#define DAT_TYPE_IMAGE   0x091E
#define RTF_TYPE_UNK1    0x08F7
#define RTF_TYPE_AUDIO   0x0514

typedef struct
	__attribute__ ((__packed__))
	{
		unsigned short header;
		unsigned short unk2;
		unsigned short unk3;
		unsigned short unk4;
		unsigned short height;
		unsigned short width;
		unsigned short unk7;
		unsigned short unk8;
		unsigned short unk9;
		unsigned short maybe_compressed;
		unsigned short unk11;
		unsigned short chunk_sizes[4];
		char *chunk[4];
	} image;

	typedef struct
		__attribute__ ((__packed__))
		{
			short header;
			short ncolors;
			int r_ptr;
			int g_ptr;
			int b_ptr;
		} palette;

#endif /* TYPES_H_ */
