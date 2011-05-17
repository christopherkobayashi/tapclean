/*
 * gremlin2.c (by Luigi Di Fraia, May 2011 - armaeth@libero.it)
 *
 * Part of project "TAPClean". May be used in conjunction with "Final TAP".
 *
 * Based on accolade.c.
 * Final TAP is (C) 2001-2006 Stewart Wilson, Subchrist Software.
 *
 *
 *  
 * This program is free software; you can redistribute it and/or modify it under 
 * the terms of the GNU General Public License as published by the Free Software 
 * Foundation; either version 2 of the License, or (at your option) any later 
 * version.
 *  
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY 
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License along with 
 * this program; if not, write to the Free Software Foundation, Inc., 51 Franklin 
 * St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

/*
 * Status: Beta
 *
 * CBM inspection needed: No
 * Single on tape: Yes
 * Sync: Byte
 * Header: Yes
 * Data: Sub-blocks (encrypted)
 * Checksum: Yes (single one at the end)
 * Post-data: No
 * Trailer: Yes
 * Trailer homogeneous: Yes (bit 1 pulses)
 */

#include "../mydefs.h"
#include "../main.h"

#define THISLOADER	GREMLINF2

#define BITSINABYTE	8	/* a byte is made up of 8 bits here */

#define SYNCSEQSIZE	1	/* amount of sync bytes */
#define MAXTRAILER	256	/* max amount of trailer pulses read in */

#define HEADERSIZE	4	/* size of block header */

#define BLOCKIDOFFSET	00	/* block ID inside header */
#define LOADOFFSETH	02	/* load location (MSB) offset inside header */
#define LOADOFFSETL	01	/* load location (LSB) offset inside header */
#define DATAOFFSETL	03	/* data size (LSB) offset inside header */

static int doffset;

static int gremlinf2_decrypt (int byte, unsigned int dest_addr)
{
	static unsigned char dblock[] = {
		0x78, 0xA9, 0xD1, 0x8D, 0xFA, 0xFF, 0x8D, 0xFE, 0xFF, 0xA9, 0x00, 0x8D, 0xFB, 0xFF, 0x8D, 0xFF,
		0xFF, 0xA9, 0x7F, 0x8D, 0x0D, 0xDC, 0x8D, 0x0D, 0xDD, 0xAD, 0x0D, 0xDC, 0xAD, 0x0D, 0xDD, 0xA9,
		0x60, 0x8D, 0x04, 0xDD, 0xA9, 0x01, 0x8D, 0x05, 0xDD, 0xA9, 0x00, 0x8D, 0x20, 0xD0, 0x85, 0xFF,
		0x85, 0xF6, 0xA9, 0x19, 0x8D, 0x0E, 0xDD, 0x20, 0xBE, 0x00, 0x66, 0xFC, 0xA5, 0xFC, 0xC9, 0xE3,
		0xD0, 0xF5, 0x20, 0xB1, 0x00, 0xC9, 0xE3, 0xF0, 0xF9, 0xC9, 0xED, 0xD0, 0xEA, 0x20, 0xB1, 0x00,
		0x85, 0xFA, 0x20, 0xB1, 0x00, 0x85, 0xFD, 0x20, 0xB1, 0x00, 0x85, 0xFE, 0x20, 0xB1, 0x00, 0x85,
		0xFB, 0xA0, 0x00, 0x20, 0xB1, 0x00, 0xA6, 0xF6, 0x55, 0x02, 0xE8, 0xE0, 0xCF, 0xD0, 0x02, 0xA2,
		0x00, 0x86, 0xF6, 0x45, 0xFE, 0x45, 0xFD, 0xC6, 0x01, 0x91, 0xFD, 0xE6, 0x01, 0x45, 0xFF, 0x85,
		0xFF, 0xE6, 0xFD, 0xD0, 0x02, 0xE6, 0xFE, 0xC6, 0xFB, 0xD0, 0xD6, 0xC6, 0xFA, 0xD0, 0xBE, 0x20,
		0xB1, 0x00, 0x85, 0xF7, 0x20, 0xB1, 0x00, 0x85, 0xF8, 0x20, 0xB1, 0x00, 0xC5, 0xFF, 0xD0, 0x03,
		0x6C, 0xF7, 0x00, 0xAD, 0x20, 0xD0, 0x18, 0x69, 0x04, 0x8D, 0x20, 0xD0, 0x4C, 0xA5, 0x00, 0xA2,
		0x08, 0x20, 0xBE, 0x00, 0x66, 0xFC, 0xCA, 0xD0, 0xF8, 0xA5, 0xFC, 0x60, 0xA9, 0x10, 0x2C, 0x0D,
		0xDC, 0xF0, 0xFB, 0x4E, 0x0D, 0xDD, 0xA9, 0x19, 0x8D, 0x0E, 0xDD, 0xEE, 0x20, 0xD0, 0x60
	};
	
	byte ^= dblock[doffset++];
	doffset %= sizeof (dblock) / sizeof (dblock[0]);

	byte ^= (dest_addr >> 8);
	byte ^= (dest_addr & 0xff);

	return byte;
}

void gremlinf2_search (void)
{
	int i, h, blk_count;		/* counters */
	int sof, sod, eod, eof, eop;	/* file offsets */
	int hd[HEADERSIZE];		/* buffer to store block header info */

	int en, tp, sp, lp, sv;		/* encoding parameters */

	unsigned int s, e;		/* block locations referred to C64 memory */
	unsigned int x; 		/* block size */

	int current_sod;		/* for current sub-block */
	unsigned int current_s, current_x;
	int current_id;

	int xinfo;			/* extra info used in addblockdef() */


	en = ft[THISLOADER].en;
	tp = ft[THISLOADER].tp;
	sp = ft[THISLOADER].sp;
	lp = ft[THISLOADER].lp;
	sv = ft[THISLOADER].sv;

	if (!quiet)
		msgout("  Gremlin F2");

	for (i = 20; i > 0 && i < tap.len - BITSINABYTE; i++) {
		eop = find_pilot(i, THISLOADER);

		if (eop > 0) {
			/* Valid pilot found, mark start of file */
			sof = i;
			i = eop;

			/* Check if there's a valid sync byte for this loader */
			if (readttbyte(i, lp, sp, tp, en) != sv)
				continue;

			/* Valid sync found, mark start of data */
			sod = i + BITSINABYTE * SYNCSEQSIZE;

			/* Mark start of current block */
			current_sod = sod;

			/* Init counters and overall size */
			blk_count = 0;
			x = 0;

			/* Cycle through all sub-blocks */
			do {
				/* Read header */
				for (h = 0; h < HEADERSIZE; h++) {
					hd[h] = readttbyte(current_sod + h * BITSINABYTE, lp, sp, tp, en);
					if (hd[h] == -1)
						break;
				}
				if (h != HEADERSIZE)
					break;

				/* Extract current sub-block ID, load location, and size */
				current_id = hd[BLOCKIDOFFSET];
				current_s = hd[LOADOFFSETL] + (hd[LOADOFFSETH] << 8);
				current_x = hd[DATAOFFSETL];

				/* Override size if required */
				if (current_x == 0)
					current_x = 0x100;

				/* Compute C64 memory location of the _LAST loaded byte_ */
				e = current_s + current_x - 1;

				/* Plausibility check */
				if (e > 0xFFFF)
					break;

				/* Point to the first pulse of the last byte (that's final) */
				eod = current_sod + (HEADERSIZE + current_x - 1) * BITSINABYTE;

				/* Initially point to the last pulse of the last byte */
				eof = eod + BITSINABYTE - 1;

				/* Move offset ahead to next block's header */
				current_sod = eof + 1;

				/* Advance count and overall size */
				blk_count++;
				x += current_x;

				/* Save overall load address */
				if (blk_count == 1)
					s = current_s;
			} while (current_id > 1);

			/* Check if we had a premature exit */
			if (current_id != 1)
				continue;

			/* Point to the first pulse of the checkbyte (that's final) */
			eod += 3 * BITSINABYTE;

			/* Initially point to the last pulse of the checkbyte (that's final) */
			eof += 3 * BITSINABYTE;

			/* Trace 'eof' to end of trailer (bit 1 pulses only) */
			h = 0;
			while (eof < tap.len - 1 &&
					h++ < MAXTRAILER &&
					readttbit(eof + 1, lp, sp, tp) == 1)
				eof++;

			/* Calculate overall end address */
			e = s + x - 1;

			/* Store the overall load/end addresses as extra-info */
			xinfo = s + (e << 16);

			if (addblockdef(THISLOADER, sof, sod, eod, eof, xinfo) >= 0)
				i = eof;	/* Search for further files starting from the end of this one */

		} else {
			if (eop < 0)
				i = (-eop);
		}
	}
}

int gremlinf2_describe (int row)
{
	int i, s, x;
	int hd[HEADERSIZE];
	int en, tp, sp, lp;
	int cb;

	int b, rd_err;

	unsigned int current_s, current_x;
	int current_id;


	en = ft[THISLOADER].en;
	tp = ft[THISLOADER].tp;
	sp = ft[THISLOADER].sp;
	lp = ft[THISLOADER].lp;

	/* Retrieve C64 memory location for overall load/end address from extra-info */
	blk[row]->cs = blk[row]->xi & 0xFFFF;
	blk[row]->ce = (blk[row]->xi & 0xFFFF0000) >> 16;
	blk[row]->cx = (blk[row]->ce - blk[row]->cs) + 1;

	/* Compute pilot & trailer lengths */

	/* pilot is in bytes... */
	blk[row]->pilot_len = (blk[row]->p2 - blk[row]->p1) / BITSINABYTE;

	/* ... trailer in pulses */
	blk[row]->trail_len = blk[row]->p4 - blk[row]->p3 - (BITSINABYTE - 1);

	/* if there IS pilot then disclude the sync byte */
	if (blk[row]->pilot_len > 0)
		blk[row]->pilot_len -= SYNCSEQSIZE;

	/* Extract data and test checksum... */
	rd_err = 0;
	cb = 0;
	x = 0;
	doffset = 0;

	s = blk[row]->p2;

	if (blk[row]->dd != NULL)
		free(blk[row]->dd);

	blk[row]->dd = (unsigned char*)malloc(blk[row]->cx);

	/* We won't get stuck in this loop for the search stage was successful */
	do {
		/* Read header (it's safe to read it here for it was already decoded during the search stage) */
		for (i = 0; i < HEADERSIZE; i++)
			hd[i]= readttbyte(s + i * BITSINABYTE, lp, sp, tp, en);

		/* Extract current sub-block ID, load location, and size */
		current_id = hd[BLOCKIDOFFSET];
		current_s = hd[LOADOFFSETL] + (hd[LOADOFFSETH] << 8);
		current_x = hd[DATAOFFSETL];

		/* Override size if required */
		if (current_x == 0)
			current_x = 0x100;

#ifdef DEBUG
		sprintf(lin,"\n - Block Number : $%02X", current_id);
		strcat(info,lin);
		sprintf(lin,"\n - Load address : $%04X", current_s);
		strcat(info,lin);
		sprintf(lin,"\n - Block size (bytes) : $%02X\n", current_x);
		strcat(info,lin);
#endif

		/* Advance to current sub-block payload */
		s += HEADERSIZE * BITSINABYTE;

		/* Do sub-block */
		for (i = 0; i < current_x; i++) {
			b = readttbyte(s + i * BITSINABYTE, lp, sp, tp, en);
			
			if (b != -1) {
				b = gremlinf2_decrypt(b, current_s + i);
				blk[row]->dd[x++] = (unsigned char) b;

				cb ^= b;
			} else {
				blk[row]->dd[x++] = 0x69;  /* error code */
				rd_err++;

				/* for experts only */
				sprintf(lin, "\n - Read Error on byte @$%X (prg data offset: $%04X)", s + (i * BITSINABYTE), i);
				strcat(info, lin);
			}
		}

		/* Advance to next sub-block header */
		s += current_x * BITSINABYTE;
	} while (current_id > 1);

	/* Read checksum that's after execution address */
	b = readttbyte(s + (2 * BITSINABYTE), lp, sp, tp, en);
	if (b == -1) {
		/* Even if not within data, we cannot validate data reliably if
		   checksum is unreadable, so that increase read errors */
		rd_err++;

		/* for experts only */
		sprintf(lin, "\n - Read Error on checkbyte @$%X", s + (i * BITSINABYTE));
		strcat(info, lin);
	}

	blk[row]->cs_exp = cb & 0xFF;
	blk[row]->cs_act = b  & 0xFF;
	blk[row]->rd_err = rd_err;

	return 0;
}
