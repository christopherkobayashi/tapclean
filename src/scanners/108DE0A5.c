/*
 * 108DE0A5.c (by Luigi Di Fraia, Aug 2006 - armaeth@libero.it)
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
 * Revision 1.4  2008/02/28 22:03:04  luigidifraia
 * Uniformed all new scanners as much as possible
 *
 * Revision 1.3  2008/02/27 20:38:04  luigidifraia
 * Removed an extra comma
 *
 * Revision 1.2  2008/01/11 00:22:20  luigidifraia
 * Uniformed code and comments in the new scanners.
 * Added read error check for unreadable checkbytes.
 *
 * Revision 1.1  2007/08/07 17:47:32  luigidifraia
 * Initial revision
 *
 */

/*
 * Status: Beta
 *
 * CBM inspection needed: No
 * Single on tape: No
 * Sync: Sequence (bytes)
 * Header: Yes
 * Data: Continuos
 * Checksum: Yes
 * Post-data: No
 * Trailer: Yes
 * Trailer homogeneous: No
 */
 
#include "../mydefs.h"
#include "../main.h"

#define THISLOADER	_108DE0A5

#define BITSINABYTE	8	/* a byte is made up of 8 bits here */

#define SYNCSEQSIZE	0x09	/* amount of sync bytes */
#define MAXTRAILER	8	/* max amount of trailer pulses read in */

#define HEADERSIZE	4	/* size of block header */

#define LOADOFFSETH	1	/* load location (MSB) offset inside header */
#define LOADOFFSETL	0	/* load location (LSB) offset inside header */
#define ENDOFFSETH	3	/* end  location (MSB) offset inside header */
#define ENDOFFSETL	2	/* end  location (LSB) offset inside header */

void _108DE0A5_search (void)
{
	int i, h;			/* counters */
	int sof, sod, eod, eof, eop;	/* file offsets */
	int pat[SYNCSEQSIZE];		/* buffer to store a sync train */
	int hd[HEADERSIZE];		/* buffer to store block header info */

	int en, tp, sp, lp, sv;		/* encoding parameters */

	unsigned int s, e;		/* block locations referred to C64 memory */
	unsigned int x; 		/* block size */

	/* legacy sync pattern */
	static int sypat[SYNCSEQSIZE] = {
		0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01
	};
	int match;			/* condition variable */


	en = ft[THISLOADER].en;
	tp = ft[THISLOADER].tp;
	sp = ft[THISLOADER].sp;
	lp = ft[THISLOADER].lp;
	sv = ft[THISLOADER].sv;

	if (!quiet)
		msgout("  108DE0A5");

	for (i = 20; i > 0 && i < tap.len - BITSINABYTE; i++) {
		eop = find_pilot(i, THISLOADER);

		if (eop > 0) {
			/* Valid pilot found, mark start of file */
			sof = i;
			i = eop;

			/* Decode a 9 byte sequence (possibly a valid sync train) */
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

			/* Extract load and end locations */
			s = hd[LOADOFFSETL] + (hd[LOADOFFSETH] << 8);
			e = hd[ENDOFFSETL]  + (hd[ENDOFFSETH]  << 8) - 1;

			/* Plausibility check */
			if (e < s)
				continue;

			/* Compute size */
			x = e - s + 1;

			/* Point to the first pulse of the checkbyte (that's final) */
			eod = sod + (HEADERSIZE + x) * BITSINABYTE;

			/* Initially point to the last pulse of the checkbyte */
			eof = eod + BITSINABYTE - 1;

			/* Trace 'eof' to end of trailer (any value, both bit 1 and bit 0 pulses) */
			h = 0;
			while (eof < tap.len - 1 && h++ < MAXTRAILER &&
					((tap.tmem[eof + 1] > sp - tol && 
					tap.tmem[eof + 1] < sp + tol) ||
					(tap.tmem[eof + 1] > lp - tol && 
					tap.tmem[eof + 1] < lp + tol)))
				eof++;

			if (addblockdef(THISLOADER, sof, sod, eod, eof, 0) >= 0)
				i = eof;	/* Search for further files starting from the end of this one */

		} else {
			if (eop < 0)
				i = (-eop);
		}
	}
}

int _108DE0A5_describe(int row)
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

	/* Extract load and end locations */
	blk[row]->cs = hd[LOADOFFSETL] + (hd[LOADOFFSETH] << 8);
	blk[row]->ce = hd[ENDOFFSETL]  + (hd[ENDOFFSETH]  << 8) - 1; /* we want the location of the last loaded byte */

	/* Compute size */
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
		cb += b;
		
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
	if (b == -1)
	{
		/* Do NOT increase read errors for this one is not within DATA, just be 
		   sure the checksum check will fail by setting it to a wrong value */
		b = ~cb;

		/* for experts only */
		sprintf(lin, "\n - Read Error on checkbyte @$%X", s + (i * BITSINABYTE));
		strcat(info, lin);
	}

	blk[row]->cs_exp = cb & 0xFF;
	blk[row]->cs_act = b  & 0xFF;
	blk[row]->rd_err = rd_err;

	return(rd_err);
}
