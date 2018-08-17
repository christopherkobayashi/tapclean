/*
 * tdi_f2.c (by Luigi Di Fraia, Aug 2006)
 * Based on tdi.c
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
 * Status: Beta
 *
 * CBM inspection needed: No
 * Single on tape: No
 * Sync: Sequence (bytes)
 * Header: Yes
 * Data: Continuous
 * Checksum: Yes
 * Post-data: Yes
 * Trailer: Yes
 * Trailer homogeneous: Yes (bit 0 pulses)
 */

#include "../mydefs.h"
#include "../main.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define THISLOADER	TDI_F2

#define BITSINABYTE	8	/* a byte is made up of 8 bits here */

#define SYNCSEQSIZE	10	/* amount of sync bytes */
#define MAXTRAILER	16	/* max amount of trailer pulses read in */

#define HEADERSIZE	4	/* size of block header (invariant part!!!) */
#define MAXNAMESIZE	4	/* max len of filename */

#define NAMEOFFSET	4	/* filename offset inside header */
#define DATAOFFSETH	1	/* data size (MSB) offset inside header */
#define DATAOFFSETL	0	/* data size (LSB) offset inside header */
#define LOADOFFSETH	3	/* load location (MSB) offset inside header */
#define LOADOFFSETL	2	/* load location (LSB) offset inside header */

#define EOFMARKER	0x20	/* EOF marker */

void tdif2_search (void)
{
	int i, h;			/* counters */
	int sof, sod, eod, eof, eop;	/* file offsets */
	int hd[HEADERSIZE];		/* buffer to store block header info */

	int en, tp, sp, lp;		/* encoding parameters */

	unsigned int s, e;		/* block locations referred to C64 memory */
	unsigned int x; 		/* block size */

	/* Expected sync pattern */
	static int sypat[SYNCSEQSIZE] = {
		0x0A, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01
	};


	en = ft[THISLOADER].en;
	tp = ft[THISLOADER].tp;
	sp = ft[THISLOADER].sp;
	lp = ft[THISLOADER].lp;

	if (!quiet)
		msgout("  TDI F2");

	for (i = 20; i > 0 && i < tap.len - BITSINABYTE; i++) {
		eop = find_pilot(i, THISLOADER);

		if (eop > 0) {
			/* Valid pilot found, mark start of file */
			sof = i;
			i = eop;

			/* Decode a SYNCSEQSIZE byte sequence (possibly a valid sync train) */
			for (h = 0; h < SYNCSEQSIZE; h++) {
				if (readttbyte(i + (h * BITSINABYTE), lp, sp, tp, en) != sypat[h])
					break;
			}

			/* Sync train doesn't match */
			if (h != SYNCSEQSIZE)
				continue;

			/* Valid sync train found, mark start of data */
			sod = i + SYNCSEQSIZE * BITSINABYTE;

			/* Read header */
			for (h = 0; h < HEADERSIZE; h++) {
				hd[h] = readttbyte(sod + h * BITSINABYTE, lp, sp, tp, en);
				if (hd[h] == -1)
					break;
			}

			/* Bail out if there was an error reading the block header */
			if (h != HEADERSIZE)
				continue;

			/* Extract load location and size */
			s = hd[LOADOFFSETL] + (hd[LOADOFFSETH] << 8);
			x = hd[DATAOFFSETL] + (hd[DATAOFFSETH] << 8);

			/* Compute C64 memory location of the _LAST loaded byte_ */
			e = s + x - 1;

			/* Plausibility check */
			if (e < s || e > 0xFFFF)
				continue;

			/* Point to the first pulse of the checkbyte (that's final) */
			eod = sod + (HEADERSIZE + x) * BITSINABYTE;

			/* Initially point to the last pulse of the checkbyte */
			eof = eod + BITSINABYTE - 1;

			/* Now TRY to add the largest block and then smaller ones */
			if (readttbyte(eof + 1 + 4 * BITSINABYTE, lp, sp, tp, en) == EOFMARKER) {
				eod += 4 * BITSINABYTE + BITSINABYTE; /* account EOF marker too */
				eof += 4 * BITSINABYTE + BITSINABYTE;

				/* Trace 'eof' to end of trailer (bit 0 pulses only) */
				h = 0;
				while (eof < tap.len - 1 &&
						h++ < MAXTRAILER &&
						readttbit(eof + 1, lp, sp, tp) == 0)
					eof++;

				if (addblockdef(THISLOADER, sof, sod, eod, eof, 4) >= 0) {
					i = eof;
					continue;
				}
			}
			if (readttbyte(eof + 1 + 2 * BITSINABYTE, lp, sp, tp, en) == EOFMARKER) {
				eod += 2 * BITSINABYTE + BITSINABYTE; /* account EOF marker too */
				eof += 2 * BITSINABYTE + BITSINABYTE;

				/* Trace 'eof' to end of trailer (bit 0 pulses only) */
				h = 0;
				while (eof < tap.len - 1 &&
						h++ < MAXTRAILER &&
						readttbit(eof + 1, lp, sp, tp) == 0)
					eof++;

				if (addblockdef(THISLOADER, sof, sod, eod, eof, 2) >= 0) {
					i = eof;
					continue;
				}
			}
			if (readttbyte(eof + 1 + 1 * BITSINABYTE, lp, sp, tp, en) == EOFMARKER) {
				eod += 1 * BITSINABYTE + BITSINABYTE; /* account EOF marker too */
				eof += 1 * BITSINABYTE + BITSINABYTE;

				/* Trace 'eof' to end of trailer (bit 0 pulses only) */
				h = 0;
				while (eof < tap.len - 1 &&
					h++ < MAXTRAILER &&
					readttbit(eof + 1, lp, sp, tp) == 0)
					eof++;

				if (addblockdef(THISLOADER, sof, sod, eod, eof, 1) >= 0)
					i = eof;
			}

		} else {
			if (eop < 0)	/* find_pilot failed (too few/many), set i to failure point. */
				i = (-eop);
		}
	}
}

int tdif2_describe (int row)
{
	int i, s;
	int hd[HEADERSIZE + MAXNAMESIZE], hdsz, fnamesz;
	int en, tp, sp, lp;
	int cb;
	char bfname[MAXNAMESIZE + 1], bfnameASCII[MAXNAMESIZE + 1];

	int b, a, rd_err;


	en = ft[THISLOADER].en;
	tp = ft[THISLOADER].tp;
	sp = ft[THISLOADER].sp;
	lp = ft[THISLOADER].lp;

	/* Set read pointer to the beginning of the payload */
	s = blk[row]->p2;

	/* Retrieve the measured filename size to know how long the header is */
	fnamesz = blk[row]->xi;

	/* Set header size accordingly */
	hdsz = HEADERSIZE + fnamesz;

	/* Read header (it's safe to read it here for it was already decoded during the search stage) */
	for (i = 0; i < hdsz; i++)
		hd[i] = readttbyte(s + i * BITSINABYTE, lp, sp, tp, en);

	/* Read filename */
	for (i = 0; i < fnamesz; i++)
		bfname[i] = hd[NAMEOFFSET + i];
	bfname[i] = '\0';

	trim_string(bfname);
	pet2text(bfnameASCII, bfname);

	if (blk[row]->fn != NULL)
		free(blk[row]->fn);
	blk[row]->fn = (char*)malloc(strlen(bfnameASCII) + 1);
	strcpy(blk[row]->fn, bfnameASCII);

	/* Read/compute C64 memory location for load/end address, and read data size */
	blk[row]->cs = hd[LOADOFFSETL] + (hd[LOADOFFSETH] << 8);
	blk[row]->cx = hd[DATAOFFSETL] + (hd[DATAOFFSETH] << 8);

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
	a = 0;

	s = blk[row]->p2 + (hdsz * BITSINABYTE);

	if(blk[row]->dd != NULL)
		free(blk[row]->dd);

	blk[row]->dd= (unsigned char*)malloc(blk[row]->cx);

	for (i = 0; i < blk[row]->cx; i++) {
		b = readttbyte(s + (i * BITSINABYTE), lp, sp, tp, en);

		if (b != -1) {
			/* decipher. */
			b ^= a;
			a = (a + 1) & 0xFF;

			cb ^= b;

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
