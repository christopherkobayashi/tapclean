/*
 * actionreplay.c (by Luigi Di Fraia, Aug 2006 - armaeth@libero.it)
 *
 * Part of project "TAPClean". May be used in conjunction with "Final TAP".
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

/*
 * $Id$
 *
 *        $Source$
 *       $RCSfile$
 *         $State$
 *      $Revision$
 *          $Date$
 *        $Author$
 *
 * $Log$
 * Revision 1.8  2009/10/18 00:55:07  luigidifraia
 * Expanded comment
 *
 * Revision 1.7  2009/10/17 20:21:11  luigidifraia
 * Uniformed sync count and comment
 *
 * Revision 1.6  2009/09/18 19:28:13  luigidifraia
 * Use readttbit to read the trailer (new scanners only atm)
 *
 * Revision 1.5  2008/02/28 22:03:04  luigidifraia
 * Uniformed all new scanners as much as possible
 *
 * Revision 1.4  2008/02/27 23:37:59  luigidifraia
 * Subtract number of sync pulses from pilot instead of bytes
 *
 * Revision 1.3  2008/02/27 20:41:03  luigidifraia
 * Removed Easytape and rewritten Action Replay scanner from scratch
 *
 */

/*
 * Status: Beta
 *
 * CBM inspection needed: No
 * Single on tape: No
 * Sync: Bit + Sequence (bytes)
 * Header: Yes
 * Data: Continuos
 * Checksum: Yes
 * Post-data: No
 * Trailer: Yes
 * Trailer homogeneous: No
 */

#include "../mydefs.h"
#include "../main.h"

#define THISLOADER	ACTIONREPLAY_HDR

#define BITSINABYTE	8	/* a byte is made up of 8 bits here */

#define SYNCSEQSIZE	2	/* amount of sync bytes */
#define MAXTRAILER	8	/* max amount of trailer pulses read in */

#define HEADERSIZE	8	/* size of block header */

#define LOADOFFSETH	2	/* load location (MSB) offset inside header */
#define LOADOFFSETL	3	/* load location (LSB) offset inside header */
#define DATAOFFSETH	0	/* data size (MSB) offset inside header */
#define DATAOFFSETL	1	/* data size (LSB) offset inside header */
#define CHKBYOFFSET	4	/* data read threshold (LSB) offset inside header */
#define DATATHRESHL	6	/* data read threshold (LSB) offset inside header */

#define DATATHRESHL_TURBO	0xD0
#define DATATHRESHL_SUPERTURBO	0x11

void ar_search (void)
{
	int i, h;			/* counters */
	int lt, dt;			/* loader/threshold used by data block */
	int sof, sod, eod, eof, eop;	/* file offsets */
	int pat[SYNCSEQSIZE];		/* buffer to store a sync train */
	int hd[HEADERSIZE];		/* buffer to store block header info */

	int en, tp, sp, lp, sv;		/* encoding parameters */

	unsigned int s, e;		/* block locations referred to C64 memory */
	unsigned int x; 		/* block size */

	int xinfo;			/* extra info used in addblockdef() */

	/* Expected sync pattern */
	static int sypat[SYNCSEQSIZE] = {
		0x52, 0x42
	};
	int match;			/* condition variable */


	en = ft[THISLOADER].en;
	tp = ft[THISLOADER].tp;
	sp = ft[THISLOADER].sp;
	lp = ft[THISLOADER].lp;
	sv = ft[THISLOADER].sv;

	if (!quiet)
		msgout("  Action Replay");

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

			/* Decode a 2 byte sequence (possibly a valid sync train) */
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

			/* Genuine AR loads at 0x0801 */
			if (s != 0x0801)
				continue;

			/* Plausibility check */
			if (e > 0xFFFF)
				continue;

			/* Additional plausibility check for this loader */
			dt = hd[DATATHRESHL];
			if (dt != DATATHRESHL_TURBO && dt != DATATHRESHL_SUPERTURBO)
				continue;

			/* Point to the first pulse of the last data byte (that's final) */
			eod = sod + (HEADERSIZE + x - 1) * BITSINABYTE;

			/* Initially point to the last pulse of the last byte */
			eof = eod + BITSINABYTE - 1;

			/* Trace 'eof' to end of trailer (any value, both bit 1 and bit 0 pulses) */
			h = 0;
			while (eof < tap.len - 1 &&
					h++ < MAXTRAILER &&
					readttbit(eof + 1, lp, sp, tp) >= 0)
				eof++;

			/* Pass details over to the describe stage */
			xinfo = dt << 24; /* threshold */
			xinfo |= (hd[CHKBYOFFSET]) << 16; /* checksum */
			xinfo |= e; /* end address */

			if (dt == DATATHRESHL_TURBO)
				lt = ACTIONREPLAY_TURBO;
			else
				lt = ACTIONREPLAY_STURBO;

			/* Acknowledge data part first */
			if (addblockdef(lt, sod + HEADERSIZE * BITSINABYTE, sod + HEADERSIZE * BITSINABYTE, eod, eof, xinfo) >= 0)
				i = eof; /* Search for further files starting from the end of this one */
			else
				continue;

			/* Acknowledge header part only if data was acked */
			addblockdef(THISLOADER, sof, sod, sod + (HEADERSIZE - 1) * BITSINABYTE, sod + (HEADERSIZE - 1) * BITSINABYTE + BITSINABYTE - 1, 0);

		} else {
			if (eop < 0)
				i = (-eop);
		}
	}
}

int ar_describe_hdr(int row)
{
	/* Just for security purposes, init to 0 the fields that do not apply */
	blk[row]->cs = 0;
	blk[row]->ce = 0;
	blk[row]->cx = 0;

	/* Compute pilot & trailer lengths (trailer will always be 0) */

	/* pilot is in pulses... */
	blk[row]->pilot_len = blk[row]->p2 - blk[row]->p1;

	/* ... trailer in pulses */
	blk[row]->trail_len = blk[row]->p4 - blk[row]->p3 - (BITSINABYTE - 1);

	/* if there IS pilot then disclude the sync sequence (1 bit + 2 bytes) */
	if (blk[row]->pilot_len > 0) 
		blk[row]->pilot_len -= SYNCSEQSIZE * BITSINABYTE + 1;

	return 0;
}

int ar_describe_data(int row)
{
	int lt, dt, i, s;
	int en, tp, sp, lp;
	int cb;

	int b, rd_err;


	/* Set load address */
	blk[row]->cs = 0x0801;

	/* Retrieve C64 memory location for end address, checksum and data type from extra-info */
	blk[row]->ce = blk[row]->xi & 0xFFFF;
	blk[row]->cs_act = (blk[row]->xi & 0x00FF0000) >> 16;
	dt = (blk[row]->xi & 0xFF000000) >> 24;

	/* Compute size */
	blk[row]->cx = blk[row]->ce - blk[row]->cs + 1;

	/* Compute pilot & trailer lengths (pilot will always be 0) */

	/* pilot is in pulses... */
	blk[row]->pilot_len = blk[row]->p2 - blk[row]->p1;

	/* ... trailer in pulses */
	blk[row]->trail_len = blk[row]->p4 - blk[row]->p3 - (BITSINABYTE - 1);

	/* if there IS pilot then disclude the sync sequence (1 bit + 2 bytes) */
	if (blk[row]->pilot_len > 0) 
		blk[row]->pilot_len -= SYNCSEQSIZE * BITSINABYTE + 1;

	/* Use the right pulsewidths to read data block */
	if (dt == DATATHRESHL_TURBO)
		lt = ACTIONREPLAY_TURBO;
	else
		lt = ACTIONREPLAY_STURBO;

	en = ft[lt].en;
	tp = ft[lt].tp;
	sp = ft[lt].sp;
	lp = ft[lt].lp;

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

	blk[row]->cs_exp = cb & 0xFF;
	blk[row]->rd_err = rd_err;

	return(rd_err);
}
