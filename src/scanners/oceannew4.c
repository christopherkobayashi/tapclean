/*
 * oceannew4.c (by Luigi Di Fraia, Aug 2006 - armaeth@libero.it)
 *
 * Part of project "TAPClean". May be used in conjunction with "Final TAP".
 *
 * Based on cyberload_f4.c, turrican.c, that are part of "Final TAP".
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
 * Sync: Byte
 * Header: Yes
 * Data: Sub-blocks
 * Checksum: Yes (inside header)
 * Post-data: No
 * Trailer: Spike
 * Trailer omogeneous: No
 */

#include "../mydefs.h"
#include "../main.h"

#define THISLOADER	OCNEW4

#define BITSINABYTE	8	/* a byte is made up of 8 bits here */

#define SYNCSEQSIZE	1	/* amount of sync bytes */
#define MAXTRAILER	8	/* max amount of trailer pulses read in */

#define HEADERSIZE	6	/* size of block header */

#define FILEIDOFFSET	0	/* filename offset inside header */
#define CHKBYOFFSET	1	/* chekcbyte offset inside header */
#define LOADOFFSETH	3	/* load location (MSB) offset inside header */
#define LOADOFFSETL	2	/* load location (LSB) offset inside header */
#define ENDOFFSETH	5	/* end  location (MSB) offset inside header */
#define ENDOFFSETL	4	/* end  location (LSB) offset inside header */


void oceannew4_search (void)
{
	int i, h;			/* counters */
	int sof, sod, eod, eof, eop;	/* file offsets */
	int hd[HEADERSIZE];		/* buffer to store block header info */

	int en, tp, sp, lp, sv;

	unsigned int s, e;		/* block locations referred to C64 memory */
	unsigned int x, bso; 		/* block size and its overload due to padding */


	en = ft[THISLOADER].en;
	tp = ft[THISLOADER].tp;
	sp = ft[THISLOADER].sp;
	lp = ft[THISLOADER].lp;
	sv = ft[THISLOADER].sv;

	if(!quiet)
		msgout("  New Ocean Tape 4");

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
			e = hd[ENDOFFSETL]  + (hd[ENDOFFSETH]  << 8) - 1;

			/* Plausibility check */
			if (e < s)
				continue;

			/* Compute size */
			x = e - s + 1;

			/* Padding must be done for this loader */
			if (x & 0xFF)
				bso = ((x & 0xFF) ^ 0xFF) + 1;

			/* Point to the first pulse of the last data byte (that's final) */
			eod = sod + (HEADERSIZE + x + bso - 1) * BITSINABYTE;

			/* Point to the last pulse of the last byte */
			eof = eod + BITSINABYTE - 1;

			/* Trace 'eof' to end of trailer (also check a different 
			   implementation that uses readttbit()) */
			/* Note: No trailer has been documented, but we are not pretending it
			         here, just checking is future-proof */
			h = 0;
			while (eof < tap.len - 1 && h++ < MAXTRAILER &&
					tap.tmem[eof + 1] > sp - tol && 
					tap.tmem[eof + 1] < sp + tol)
				eof++;

			if (addblockdef(THISLOADER, sof, sod, eod, eof, 0) >= 0)
				i = eof;	/* Search for further files starting from the end of this one */
			else {
				/* Try the variant (the one with no data padding) */

				/* Point to the first pulse of the last data byte (that's final) */
				eod = sod + (HEADERSIZE + x - 1) * BITSINABYTE;

				/* Point to the last pulse of the last byte */
				eof = eod + BITSINABYTE - 1;

				/* Trace 'eof' to end of trailer (also check a different 
				   implementation that uses readttbit()) */
				/* Note: No trailer has been documented, but we are not pretending it
				         here, just checking for it is future-proof */
				h = 0;
				while (eof < tap.len - 1 && h++ < MAXTRAILER &&
						tap.tmem[eof + 1] > sp - tol && 
						tap.tmem[eof + 1] < sp + tol)
					eof++;

				if (addblockdef(THISLOADER, sof, sod, eod, eof, 0) >= 0)
					i = eof;	/* Search for further files starting from the end of this one */
			}
		} else {
			if (eop < 0)
				i = (-eop);
		}
	}
}

int oceannew4_describe (int row)
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

	/* Read header */
	for (i = 0; i < HEADERSIZE; i++)
		hd[i]= readttbyte(s + i * BITSINABYTE, lp, sp, tp, en);

	sprintf(lin,"\n - Block Number : $%02X", hd[FILEIDOFFSET]);
	strcat(info,lin);

	/* Extract load and end locations */
	blk[row]->cs = hd[LOADOFFSETL] + (hd[LOADOFFSETH] << 8);
	blk[row]->ce = hd[ENDOFFSETL]  + (hd[ENDOFFSETH]  << 8) - 1; /* we want the location of the last loaded byte */

	/* Compute size (no padding, just extract the advertised amount of bytes) */
	blk[row]->cx = blk[row]->ce - blk[row]->cs + 1;

	/* Compute pilot & trailer lengths */

	/* pilot is in bytes... */
	blk[row]->pilot_len = (blk[row]->p2 - blk[row]->p1) / BITSINABYTE;

	/* ... trailer in pulses */
	/* Note: No trailer has been documented, but we are not pretending it
	         here, just checking for it is future-proof */
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
			sprintf(lin, "\n - Read Error on byte @$%X (prg data offset: $%04X)", s + (i * BITSINABYTE), i);
			strcat(info, lin);
		}
	}
	b = hd[CHKBYOFFSET];

	blk[row]->cs_exp = cb & 0xFF;
	blk[row]->cs_act = b;
	blk[row]->rd_err = rd_err;

	return(rd_err);
}