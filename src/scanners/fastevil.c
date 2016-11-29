/*
 * fastevil.c (by Luigi Di Fraia, Sep 2009)
 *
 * Part of project "TAPClean". May be used in conjunction with "Final TAP".
 *
 * Based on ashdave.c, which is part of "TAPClean".
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
 * Data: Continuous
 * Checksum: No
 * Post-data: Yes
 * Trailer: No
 * Trailer homogeneous: N/A
 */

/*
 * Note: Due to loss of precision in the TAP format, the threshold for this
 *       loader has been set to 0x1D rather than 0x1E. Some bit 1 pulses
 *       have been found shifted down to 0x1E.
 */

#include "../mydefs.h"
#include "../main.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define THISLOADER	FASTEVIL

#define BITSINABYTE	8	/* a byte is made up of 8 bits here */

#define SYNCSEQSIZE	1	/* amount of sync bytes */
#define MAXTRAILER	8	/* max amount of trailer pulses read in */

#define HEADERSIZE	6	/* size of block header */

#define LOADOFFSETH	0	/* load location (MSB) offset inside header */
#define LOADOFFSETL	1	/* load location (LSB) offset inside header */
#define ENDOFFSETH	2	/* end  location (MSB) offset inside header */
#define ENDOFFSETL	3	/* end  location (LSB) offset inside header */

#define POSTDATASIZE	256	/* size in bytes of the MANDATORY information
				   that is found after file data */

void fastevil_search (void)
{
	int i, h;			/* counters */
	int sof, sod, eod, eof, eop;	/* file offsets */
	int hd[HEADERSIZE];		/* buffer to store block header info */
	int pat[POSTDATASIZE];		/* buffer to store postdata pattern */

	int en, tp, sp, lp, sv;		/* encoding parameters */

	unsigned int s, e;		/* block locations referred to C64 memory */
	unsigned int x; 		/* block size */

	/* Expected postdata pattern */
	static int sypat[POSTDATASIZE] = {
		0x00,0xFF,0xFE,0xFD,0xFC,0xFB,0xFA,0xF9,0xF8,0xF7,0xF6,0xF5,0xF4,0xF3,0xF2,0xF1,
		0xF0,0xEF,0xEE,0xED,0xEC,0xEB,0xEA,0xE9,0xE8,0xE7,0xE6,0xE5,0xE4,0xE3,0xE2,0xE1,
		0xE0,0xDF,0xDE,0xDD,0xDC,0xDB,0xDA,0xD9,0xD8,0xD7,0xD6,0xD5,0xD4,0xD3,0xD2,0xD1,
		0xD0,0xCF,0xCE,0xCD,0xCC,0xCB,0xCA,0xC9,0xC8,0xC7,0xC6,0xC5,0xC4,0xC3,0xC2,0xC1,
		0xC0,0xBF,0xBE,0xBD,0xBC,0xBB,0xBA,0xB9,0xB8,0xB7,0xB6,0xB5,0xB4,0xB3,0xB2,0xB1,
		0xB0,0xAF,0xAE,0xAD,0xAC,0xAB,0xAA,0xA9,0xA8,0xA7,0xA6,0xA5,0xA4,0xA3,0xA2,0xA1,
		0xA0,0x9F,0x9E,0x9D,0x9C,0x9B,0x9A,0x99,0x98,0x97,0x96,0x95,0x94,0x93,0x92,0x91,
		0x90,0x8F,0x8E,0x8D,0x8C,0x8B,0x8A,0x89,0x88,0x87,0x86,0x85,0x84,0x83,0x82,0x81,
		0x80,0x7F,0x7E,0x7D,0x7C,0x7B,0x7A,0x79,0x78,0x77,0x76,0x75,0x74,0x73,0x72,0x71,
		0x70,0x6F,0x6E,0x6D,0x6C,0x6B,0x6A,0x69,0x68,0x67,0x66,0x65,0x64,0x63,0x62,0x61,
		0x60,0x5F,0x5E,0x5D,0x5C,0x5B,0x5A,0x59,0x58,0x57,0x56,0x55,0x54,0x53,0x52,0x51,
		0x50,0x4F,0x4E,0x4D,0x4C,0x4B,0x4A,0x49,0x48,0x47,0x46,0x45,0x44,0x43,0x42,0x41,
		0x40,0x3F,0x3E,0x3D,0x3C,0x3B,0x3A,0x39,0x38,0x37,0x36,0x35,0x34,0x33,0x32,0x31,
		0x30,0x2F,0x2E,0x2D,0x2C,0x2B,0x2A,0x29,0x28,0x27,0x26,0x25,0x24,0x23,0x22,0x21,
		0x20,0x1F,0x1E,0x1D,0x1C,0x1B,0x1A,0x19,0x18,0x17,0x16,0x15,0x14,0x13,0x12,0x11,
		0x10,0x0F,0x0E,0x0D,0x0C,0x0B,0x0A,0x09,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01
	};
	int match;			/* condition variable */


	en = ft[THISLOADER].en;
	tp = ft[THISLOADER].tp;
	sp = ft[THISLOADER].sp;
	lp = ft[THISLOADER].lp;
	sv = ft[THISLOADER].sv;

	if (!quiet)
		msgout("  Fast Evil");

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

			/* Read header */
			for (h = 0; h < HEADERSIZE; h++) {
				hd[h] = readttbyte(sod + h * BITSINABYTE, lp, sp, tp, en);
				if (hd[h] == -1)
					break;
			}
			if (h != HEADERSIZE)
				continue;

			/* Extract load and end locations */
			s = hd[LOADOFFSETL] + (hd[LOADOFFSETH] << 8);
			e = hd[ENDOFFSETL]  + (hd[ENDOFFSETH]  << 8);

			/* Prevent int wraparound when subtracting 1 from end location
			   to get the location of the last loaded byte */
			if (e == 0)
				e = 0xFFFF;
			else
				e--;

			/* Plausibility check */
			if (e < s)
				continue;

			/* Compute size */
			x = e - s + 1;

			/* Point to the first pulse of the last data byte (that's final) */
			eod = sod + (HEADERSIZE + x - 1) * BITSINABYTE;

			/* Point to the first pulse of the first postdata byte */
			eof = eod + BITSINABYTE;

			/* Decode a 256 byte sequence (possibly a valid postdata pattern) */
			for (h = 0; h < POSTDATASIZE; h++)
				pat[h] = readttbyte(eof + (h * BITSINABYTE), lp, sp, tp, en);

			/* Note: no need to check if readttbyte is returning -1, for
			         the following comparison (DONE ON ALL READ BYTES)
			         will fail all the same in that case */

			/* Check postdata pattern. We may use the find_seq() facility too */
			for (match = 1, h = 0; h < POSTDATASIZE; h++)
				if (pat[h] != sypat[h])
					match = 0;

			/* Postdata pattern doesn't match */
			if (!match)
				continue;

			/* Point to the last pulse of the last postdata byte */
			eof = eod + (POSTDATASIZE + 1) * BITSINABYTE - 1;

			/* Trace 'eof' to end of trailer (any value, both bit 1 and bit 0 pulses) */
			/* Note: No trailer has been documented, but we are not strictly
			         requiring one here, just checking for it is future-proof */
			h = 0;
			while (eof < tap.len - 1 &&
					h++ < MAXTRAILER &&
					readttbit(eof + 1, lp, sp, tp) >= 0)
				eof++;

			if (addblockdef(THISLOADER, sof, sod, eod, eof, 0) >= 0)
				i = eof;	/* Search for further files starting from the end of this one */

		} else {
			if (eop < 0)
				i = (-eop);
		}
	}
}

int fastevil_describe (int row)
{
	int i, s;
	int hd[HEADERSIZE];
	int en, tp, sp, lp;

	int b, rd_err;


	en = ft[THISLOADER].en;
	tp = ft[THISLOADER].tp;
	sp = ft[THISLOADER].sp;
	lp = ft[THISLOADER].lp;

	/* Note: addblockdef() is the glue between ft[] and blk[], so we can now read from blk[] */
	s = blk[row] -> p2;

	/* Read header (it's safe to read it here for it was already decoded during the search stage) */
	for (i = 0; i < HEADERSIZE; i++)
		hd[i]= readttbyte(s + i * BITSINABYTE, lp, sp, tp, en);

	/* Extract load and end locations */
	blk[row]->cs = hd[LOADOFFSETL] + (hd[LOADOFFSETH] << 8);
	blk[row]->ce = hd[ENDOFFSETL]  + (hd[ENDOFFSETH]  << 8);

	/* Prevent int wraparound when subtracting 1 from end location
	   to get the location of the last loaded byte */
	if (blk[row]->ce == 0)
		blk[row]->ce = 0xFFFF;
	else
		(blk[row]->ce)--;

	/* Compute size */
	blk[row]->cx = blk[row]->ce - blk[row]->cs + 1;

	/* Compute pilot & trailer lengths */

	/* pilot is in bytes... */
	blk[row]->pilot_len = (blk[row]->p2 - blk[row]->p1) / BITSINABYTE;

	/* ... trailer in pulses */
	blk[row]->trail_len = blk[row]->p4 - blk[row]->p3 - (BITSINABYTE - 1);

	/* if there IS pilot then disclude the sync byte */
	if (blk[row]->pilot_len > 0)
		blk[row]->pilot_len -= SYNCSEQSIZE;

	/* Extract data */
	rd_err = 0;

	s = blk[row]->p2 + (HEADERSIZE * BITSINABYTE);

	if (blk[row]->dd != NULL)
		free(blk[row]->dd);

	blk[row]->dd = (unsigned char*)malloc(blk[row]->cx);

	for (i = 0; i < blk[row]->cx; i++) {
		b = readttbyte(s + (i * BITSINABYTE), lp, sp, tp, en);
		if (b != -1) {
			blk[row]->dd[i] = b;
		} else {
			blk[row]->dd[i] = 0x69;  /* error code */
			rd_err++;

			/* for experts only */
			sprintf(lin, "\n - Read Error on byte @$%X (prg data offset: $%04X)", s + (i * BITSINABYTE), i + 2);
			strcat(info, lin);
		}
	}

	blk[row]->rd_err = rd_err;

	return(rd_err);
}
