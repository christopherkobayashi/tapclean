/*
 * cult.c (by slc, bgk, and luigi)
 *
 * Part of project "TAPClean". May be used in conjunction with "Final TAP".
 *
 *
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

#include "../mydefs.h"
#include "../main.h"

#define THISLOADER	CULT

#define BITSINABYTE	8	/* a byte is made up of 8 bits here */

#define SPARESYNCVAL	0xAA	/* value of the spare sync byte */
#define SPRSYNCSEQSIZE	1	/* amount of spare sync bytes */

#define LOADOFFSETH	0x0E	/* load location (MSB) offset inside CBM data */
#define LOADOFFSETL	0x0A	/* load location (LSB) offset inside CBM data */
#define ENDOFFSETH	0x1F	/* end location (MSB) offset inside CBM header */
#define ENDOFFSETL	0x1B	/* end location (LSB) offset inside CBM header */

void cult_search (void)
{
	int i;				/* counter */
	int sof, sod, eod, eof, eop;	/* file offsets */
	int ib;				/* condition variable */

	int en, tp, sp, lp, sv;

	unsigned int s, e;		/* block locations referred to C64 memory */
	unsigned int x; 		/* block size */

	int xinfo;			/* extra info used in addblockdef() */


	en = ft[THISLOADER].en;
	tp = ft[THISLOADER].tp;
	sp = ft[THISLOADER].sp;
	lp = ft[THISLOADER].lp;
	sv = ft[THISLOADER].sv;

	if (!quiet)
		msgout("  Cult tape");

	/* First we retrieve the Cult variables from the CBM header and data */
	ib = find_decode_block(CBM_DATA, 1);
	if (ib == -1)
		return;		/* failed to locate cbm data. */

	s = blk[ib]->dd[LOADOFFSETL] + (blk[ib]->dd[LOADOFFSETH] << 8); /* 0x0801 */

	ib = find_decode_block(CBM_HEAD, 1);
	if (ib == -1)
		return;		/* failed to locate cbm header. */

	e = blk[ib]->dd[ENDOFFSETL] + (blk[ib]->dd[ENDOFFSETH] << 8) - 1;

	/* Plausibility checks (here since CULT is always just ONE TURBO file) */
	/* Note: a plausibility check is on s == 0x0801 because load address
		 is stored in CBM data, which is the very same file for all 
		 genuine Cult tapes! */
	if (e < s || s != 0x0801)
		return;

	/* Note: we may exit the "for" cycle if addblockdef() doesn't fail,
	   since CULT is always just ONE turbo file. I didn't do that because 
	   we may have more than one game on the same tape using Cult loader */
	for (i = 20; i > 0 && i < tap.len - BITSINABYTE; i++) {
		eop = find_pilot(i, THISLOADER);

		if (eop > 0) {
			/* Valid pilot found, mark start of file */
			sof = i;
			i = eop;

			/* Check if there's a valid sync bit for this loader */
			if (readttbit(i, lp, sp, tp) != sv)
				continue;

			i++; /* Take into account this bit */

			/* Check if there's a valid spare sync byte for this loader */
			if (readttbyte(i, lp, sp, tp, en) != SPARESYNCVAL)
				continue;

			/* Valid sync bit + sync byte found, mark start of data */
			sod = i + BITSINABYTE * SPRSYNCSEQSIZE;

			/* Compute size */
			x = e - s + 1;

			/* Point to the first pulse of the last data byte (that's final) */
			eod = sod + (x - 1) * BITSINABYTE;

			/* Point to the last pulse of the last byte */
			eof = eod + BITSINABYTE - 1;

			/* Trace 'eof' to end of trailer (also check a different 
			   implementation that uses readttbit()) */
			while (eof < tap.len - 1 && 
					tap.tmem[eof + 1] > sp - tol && 
					tap.tmem[eof + 1] < sp + tol)
				eof++;

			/* Also account the bit 1 trailer pulse, if any */
			if (eof > 0 && eof < tap.len - 1 && 
					tap.tmem[eof + 1] > lp - tol && 
					tap.tmem[eof + 1] < lp + tol)
				eof++;

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

int cult_describe(int row)
{
	int i, s;
	int en, tp, sp, lp;

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

	/* don't forget sync is 9 bits... */
	blk[row]->pilot_len = (blk[row]->p2 - blk[row]->p1 - 1 - BITSINABYTE);
	blk[row]->trail_len = blk[row]->p4 - blk[row]->p3 - (BITSINABYTE - 1);

	/* Extract data */
	rd_err = 0;

	s = blk[row]->p2;

	if (blk[row]->dd != NULL)
		free(blk[row]->dd);

	blk[row]->dd = (unsigned char*)malloc(blk[row]->cx);

	for (i = 0; i < blk[row]->cx; i++) {
		b = readttbyte(s + (i * BITSINABYTE), lp, sp, tp, en);
		if(b != -1) {
			blk[row]->dd[i] = b;
		} else {
			blk[row]->dd[i] = 0x69;  /* error code */
			rd_err++;

			/* for experts only */
			sprintf(lin, "\n - Read Error on byte @$%X (prg data offset: $%04X)", s + (i * BITSINABYTE), i);
			strcat(info, lin);
		}
	}

	blk[row]->rd_err = rd_err;

	return(rd_err);
}
