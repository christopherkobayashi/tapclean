/*
 * rainbowarts_f1.c (by Luigi Di Fraia, Aug 2006)
 *
 * Part of project "TAPClean". May be used in conjunction with "Final TAP".
 *
 * Based on virgin.c, part of "Final TAP".
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
 * Single on tape: No
 * Sync: Sequence (bytes)
 * Header: Yes
 * Data: Continuous
 * Checksum: Yes
 * Post-data: No
 * Trailer: Yes
 * Trailer homogeneous: No
 */

#include "../mydefs.h"
#include "../main.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define THISLOADER	RAINBOWARTS_F1

#define BITSINABYTE	8	/* a byte is made up of 8 bits here */

#define SYNCSEQSIZE	10	/* amount of sync bytes */
#define MAXTRAILER	8	/* max amount of trailer pulses read in */

#define HEADERSIZE	5	/* size of block header */

#define FILEIDOFFSET	0	/* filename offset inside header */
#define LOADOFFSETH	2	/* load location (MSB) offset inside header */
#define LOADOFFSETL	1	/* load location (LSB) offset inside header */
#define DATAOFFSETH	4	/* data size (MSB) offset inside header */
#define DATAOFFSETL	3	/* data size (LSB) offset inside header */

void rainbowf1_search (void)
{
	int i, h;			/* counters */
	int sof, sod, eod, eof, eop;	/* file offsets */
	int pat[SYNCSEQSIZE];		/* buffer to store a sync train */
	int hd[HEADERSIZE];		/* buffer to store block header info */

	int en, tp, sp, lp;		/* encoding parameters */

	unsigned int s, e;		/* block locations referred to C64 memory */
	unsigned int x; 		/* block size */

	/* Expected sync pattern */
	static int sypat[SYNCSEQSIZE] = {
		0x0A, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01
	};
	int match;			/* condition variable */


	en = ft[THISLOADER].en;
	tp = ft[THISLOADER].tp;
	sp = ft[THISLOADER].sp;
	lp = ft[THISLOADER].lp;

	if (!quiet)
		msgout("  Rainbow Arts F1");

	for (i = 20; i > 0 && i < tap.len - BITSINABYTE; i++) {
		eop = find_pilot(i, THISLOADER);

		if (eop > 0) {
			/* Valid pilot found, mark start of file */
			sof = i;
			i = eop;

			/* Decode a SYNCSEQSIZE byte sequence (possibly a valid sync train) */
			for (h = 0; h < SYNCSEQSIZE; h++)
				pat[h] = readttbyte(i + (h * BITSINABYTE), lp, sp, tp, en);

			/* Note: no need to check if readttbyte is returning -1, for
			         the following comparison (DONE ON ALL READ BYTES)
				 will fail all the same in that case */

			/* Check sync train. We may use the find_seq() facility too */
			for (match = 1, h = 0; h < SYNCSEQSIZE; h++)
				if (pat[h] != sypat[h])
					match = 0;

			/* Sync train doesn't match */
			if (!match)
				continue;

			/* Valid sync train found, mark start of data */
			sod = i + BITSINABYTE * SYNCSEQSIZE;

			/* Read header */
			for (h = 0; h < HEADERSIZE; h++) {
				hd[h] = readttbyte(sod + h * BITSINABYTE, lp, sp, tp, en);
				if (hd[h] == -1)
					break;
			}
			if (h != HEADERSIZE)
				continue;

			/* Extract load location and size */
			s = hd[LOADOFFSETL] + (hd[LOADOFFSETH] << 8);
			x = 0x10000 - (hd[DATAOFFSETL] + (hd[DATAOFFSETH] << 8));

			/* Compute C64 memory location of the _LAST loaded byte_ */
			e = s + x - 1;

			/* Plausibility check */
			if (e > 0xFFFF)
				continue;

			/* Point to the first pulse of the checkbyte (that's final) */
			eod = sod + (HEADERSIZE + x) * BITSINABYTE;

			/* Initially point to the last pulse of the checkbyte */
			eof = eod + BITSINABYTE - 1;

			/* Trace 'eof' to end of trailer (any value, both bit 1 and bit 0 pulses) */
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

int rainbowf1_describe (int row)
{
	int i, s;
	int hd[HEADERSIZE];
	int en, tp, sp, lp;
	int cb;

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

	sprintf(lin,"\n - Block Number : $%02X", hd[FILEIDOFFSET]);
	strcat(info,lin);

	/* Read/compute C64 memory location for load/end address, and read data size */
	blk[row]->cs = hd[LOADOFFSETL] + (hd[LOADOFFSETH] << 8);
	blk[row]->cx = 0x10000 - (hd[DATAOFFSETL] + (hd[DATAOFFSETH] << 8));

	/* C64 memory location of the _LAST loaded byte_ */
	blk[row]->ce = blk[row]->cs + blk[row]->cx - 1;

	/* Compute pilot & trailer lengths */

	/* pilot is in bytes... */
	blk[row]->pilot_len = (blk[row]->p2 - blk[row]->p1) / BITSINABYTE;

	/* ... trailer in pulses */
	blk[row]->trail_len = blk[row]->p4 - blk[row]->p3 - (BITSINABYTE - 1);

	/* if there IS pilot then disclude the sync sequence */
	if (blk[row]->pilot_len > 0)
		blk[row]->pilot_len -= SYNCSEQSIZE;

	/* Extract data and test checksum... */
	rd_err = 0;
	cb = 0;

	s = blk[row]->p2 + (HEADERSIZE * BITSINABYTE);

	if (blk[row]->dd != NULL)
		free(blk[row]->dd);

	blk[row]->dd = (unsigned char*)malloc(blk[row]->cx);

	for (i = 0; i < blk[row]->cx; i++) {
		b = readttbyte(s + (i * BITSINABYTE), lp, sp, tp, en);
		cb ^= b;

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

	b = readttbyte(s + (i * BITSINABYTE), lp, sp, tp, en);
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

	return(rd_err);
}
