/*
 * powerload.c (by Luigi Di Fraia, May 2011 - armaeth@libero.it)
 *
 * Part of project "TAPClean". May be used in conjunction with "Final TAP".
 *
 * Based on biturbo.c and cult.c.
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
 * CBM inspection needed: Yes
 * Single on tape:  Yes! -> once we acknowledge one, we can return
 * Sync: Sequence (bytes)
 * Header: No
 * Data: Continuous
 * Checksum: Yes (after post-data)
 * Post-data: Yes
 * Trailer: Yes
 * Trailer homogeneous: Yes (bit 0 pulses)
 */

#include "../mydefs.h"
#include "../main.h"

#define THISLOADER	POWERLOAD

#define BITSINABYTE	8	/* a byte is made up of 8 bits here */

#define SYNCSEQSIZE	10	/* amount of sync bytes */
#define MAXTRAILER	2040	/* max amount of trailer pulses read in */
#define MAXPOSTDATASIZE	(BITSINABYTE * 0x01D0)	/* Max amount of post-data pulses */

#define LOADOFFSETH	0x03	/* load location (MSB) offset inside CBM data */
#define LOADOFFSETL	0x01	/* load location (LSB) offset inside CBM data */
#define ENDOFFSETH	0x0B	/* end location (MSB) offset inside CBM data */
#define ENDOFFSETL	0x09	/* end location (LSB) offset inside CBM data */

void powerload_search (void)
{
	int i, h, post;			/* counters */
	int sof, sod, eod, eof, eop;	/* file offsets */
	int pat[SYNCSEQSIZE];		/* buffer to store a sync train */
	int ib;				/* condition variable */

	int en, tp, sp, lp, sv;		/* encoding parameters */

	unsigned int s, e;		/* block locations referred to C64 memory */
	unsigned int x; 		/* block size */

	/* Expected sync pattern */
	static int sypat[SYNCSEQSIZE] = {
		0x09,0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00
	};
	int match;			/* condition variable */

	int xinfo;			/* extra info used in addblockdef() */


	en = ft[THISLOADER].en;
	tp = ft[THISLOADER].tp;
	sp = ft[THISLOADER].sp;
	lp = ft[THISLOADER].lp;
	sv = ft[THISLOADER].sv;

	if (!quiet)
		msgout("  Power Load");

	/* First we retrieve the Power Load variables from the CBM data */
	ib = find_decode_block(CBM_DATA, 3);
	if (ib == -1)
		return;		/* failed to locate cbm data. */

	/* Basic validation before accessing array elements */
	if (blk[ib]->cx < ENDOFFSETH + 1)
		return;

	s = blk[ib]->dd[LOADOFFSETL] + (blk[ib]->dd[LOADOFFSETH] << 8);
	e = blk[ib]->dd[ENDOFFSETL] + (blk[ib]->dd[ENDOFFSETH] << 8);

	/* Prevent int wraparound when subtracting 1 from end location
	   to get the location of the last loaded byte */
	if (e == 0)
		e = 0xFFFF;
	else
		e--;

	/* Plausibility checks (here since POWERLOAD is always just ONE TURBO file) */
	if (e < s)
		return;

	for (i = 20; i > 0 && i < tap.len - BITSINABYTE; i++) {
		eop = find_pilot(i, THISLOADER);

		if (eop > 0) {
			/* Valid pilot found, mark start of file */
			sof = i;
			i = eop;

			/* Decode a 10 byte sequence (possibly a valid sync train) */
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

			/* Compute size */
			x = e - s + 1;

			/* Point to the first pulse of the last byte (that's final) */
			eod = sod + (x - 1) * BITSINABYTE;

			/* Initially point to the last pulse of the last byte */
			eof = eod + BITSINABYTE - 1;

			/* Trace 'eof' to end of post-data and trailer */
			h = 0;
			while (eof < tap.len - 1 &&
					h++ < MAXTRAILER + MAXPOSTDATASIZE &&
					readttbit(eof + 1, lp, sp, tp) >= 0)
				eof++;

			/* Trace back to the checkbyte position */
			post = eof - (eof - eod) % BITSINABYTE;	/* Byte boundary */

			while (post > eod + BITSINABYTE &&
					readttbyte(post - BITSINABYTE, lp, sp, tp, en) == 0)
				post -= BITSINABYTE;

			/* Extend data block to include post-data */
			e += (post - eod) / BITSINABYTE - 2;
			if (e < s)
				continue;

			/* Extend block info too (point to first pulse of the checkbyte) */
			eod = post - BITSINABYTE;

			/* Store the info read from CBM part as extra-info */
			xinfo = s + (e << 16);

			if (addblockdef(THISLOADER, sof, sod, eod, eof, xinfo) >= 0)
				i = eof;	/* Search for further files starting from the end of this one */

		} else {
			if (eop < 0)
				i = (-eop);
		}
	}
}

int powerload_describe(int row)
{
	int i, s;
	int en, tp, sp, lp;
	int cb;

	int b, rd_err;


	en = ft[THISLOADER].en;
	tp = ft[THISLOADER].tp;
	sp = ft[THISLOADER].sp;
	lp = ft[THISLOADER].lp;

	/* Retrieve C64 memory location for load/end address from extra-info */
	blk[row]->cs = blk[row]->xi & 0xFFFF;
	blk[row]->ce = (blk[row]->xi & 0xFFFF0000) >> 16;
	blk[row]->cx = (blk[row]->ce - blk[row]->cs) + 1;

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

	s = blk[row]->p2;

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
			sprintf(lin, "\n - Read Error on byte @$%X (prg data offset: $%04X)", s + (i * BITSINABYTE), i);
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
