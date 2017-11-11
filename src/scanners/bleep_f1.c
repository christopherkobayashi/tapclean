/*
 * bleep_f1.c (rewritten by Luigi Di Fraia, Nov 2017)
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
 * Post-data: Yes for last file (Bleepload Trigger)
 * Trailer: Yes for last file (Bleepload Trigger)
 * Trailer homogeneous: Yes (bit 1 pulses)
 */

#include "../mydefs.h"
#include "../main.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define THISLOADER	BLEEP

#define BITSINABYTE	8	/* a byte is made up of 8 bits here */

#define SYNCSEQSIZE	2	/* amount of sync bytes */
#define MAXTRAILER	2040	/* max amount of trailer pulses read in */

#define HEADERSIZE	4	/* size of block header */

#define LOADOFFSETL	2	/* load location (LSB) offset inside header */
#define LOADOFFSETH	3	/* load location (MSB) offset inside header */
#define NEXTPILOTOFFSET	0	/* next block pilot value offset inside header */
#define BLOCKIDOFFSET	1	/* file ID offset inside header */

void bleep_search(void)
{
	int i, h;			/* counters */
	int sof, sod, eod, eof, eop;	/* file offsets */
	int hd[HEADERSIZE];		/* buffer to store block header info */

	int en, tp, sp, lp, pv;		/* encoding parameters */

	unsigned int x; 		/* block size */

	int xinfo;			/* extra info used in addblockdef() */

	int sypat[SYNCSEQSIZE];		/* expected sync pattern */


	en = ft[THISLOADER].en;
	tp = ft[THISLOADER].tp;
	sp = ft[THISLOADER].sp;
	lp = ft[THISLOADER].lp;
	pv = 0x0F;			/* initial pilot value */

	if (!quiet)
		msgout("  Bleepload");

	for (i = 20; i > 0 && i < tap.len - BITSINABYTE; i++) {
		ft[THISLOADER].pv = pv;
		ft[THISLOADER].sv = pv ^ 0xFF;

		eop = find_pilot(i, THISLOADER);

		if (eop > 0) {
			/* Valid pilot found, mark start of file */
			sof = i;
			i = eop;

			sypat[0] = pv ^ 0xFF;
			sypat[1] = pv;

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

			/* Extract pilot value for next block */
			pv = hd[NEXTPILOTOFFSET];

			/* Plausibility check: if 0, this could actually be Bleepload Special */
			if (pv == 0)
				continue;

			/* Set size: 64 bytes for file with ID 0, 256 for others */
			if (hd[BLOCKIDOFFSET] == 0)
				x = 0x40;
			else
				x = 0x100;

			/* Point to the first pulse of the last data byte (that's final) */
			eod = sod + (HEADERSIZE + x - 1) * BITSINABYTE;

			/* Point to the last pulse of the last byte */
			eof = eod + BITSINABYTE - 1;

			/* Read the number of dummy bytes and validate the read */
			xinfo = readttbyte(eof + 1, lp, sp, tp, en);
			if (xinfo < 0)
				continue;

			/* Displace ends by the count of dummy bytes, dummy bytes, checksum, exe address */
			eod += (1 + xinfo + 1 + 2) * BITSINABYTE;
			eof += (1 + xinfo + 1 + 2) * BITSINABYTE;

			if (addblockdef(THISLOADER, sof, sod, eod, eof, xinfo) >= 0) {
				i = eof;	/* Search for further files starting from the end of this one */

				if (readttbyte(eof + 1, lp, sp, tp, en) == 0) {
					sof = eof + 1;
					sod = sof;
					eod = sof + 56;
					eof = sof + 63;

					/* Trace 'eof' to end of trailer (bit 1 pulses only) */
					h = 0;
					while (eof < tap.len - 1 &&
							h++ < MAXTRAILER &&
							readttbit(eof + 1, lp, sp, tp) == 1)
						eof++;

					if (addblockdef(BLEEP_TRIG, sof, sod, eod, eof, 0) >= 0)
						i = eof;
				}
			}

		} else {
			if (eop < 0)	/* find_pilot failed (too few/many), set i to failure point. */
				i = (-eop);
		}
	}
}

int bleep_describe(int row)
{
	int i, j, s;
	int dummy_cnt;
	unsigned int exe_address;
	int hd[HEADERSIZE];
	int en, tp, sp, lp;
	int cb;

	int b, rd_err;


	en = ft[THISLOADER].en;
	tp = ft[THISLOADER].tp;
	sp = ft[THISLOADER].sp;
	lp = ft[THISLOADER].lp;

	if(blk[row]->lt == BLEEP_TRIG) {
		s = blk[row]->p2;

		/* set pilot/trailer lengths... */
		blk[row]->pilot_len = 0;
		blk[row]->trail_len = blk[row]->p4 - (blk[row]->p3 + BITSINABYTE - 1);

		sprintf(lin, "\n - Data length : 8 bytes (always)");
		strcat(info, lin);

		strcat(info, "\n - Contents : ");
		for (i = 0; i < 8; i++) {
			b = readttbyte(s + (i * BITSINABYTE), lp, sp, tp, en);

			// Do NOT increase read errors for this one is not within DATA
			if (b == -1)
				strcat(info, "-- ");
			else {
				sprintf(lin, "%02X ", b);
				strcat(info, lin);
			}
		}

		blk[row]->rd_err = 0;

		return 0;
	}

	/* Note: addblockdef() is the glue between ft[] and blk[], so we can now read from blk[] */
	s = blk[row]->p2;

	/* Read header (it's safe to read it here for it was already decoded during the search stage) */
	for (i = 0; i < HEADERSIZE; i++)
		hd[i] = readttbyte(s + i * BITSINABYTE, lp, sp, tp, en);

	/* Extract load location */
	blk[row]->cs = hd[LOADOFFSETL] + (hd[LOADOFFSETH] << 8);

	/* Set size: 64 bytes for file with ID 0, 256 for others */
	if (hd[BLOCKIDOFFSET] == 0)
		blk[row]->cx = 0x40;
	else
		blk[row]->cx = 0x100;

	/* C64 memory location of the _LAST loaded byte_ */
	blk[row]->ce = blk[row]->cs + blk[row]->cx - 1;

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

	/* Extract block ID and pilot value and print them out */
	sprintf(lin, "\n - Block ID : $%02X", hd[BLOCKIDOFFSET]);
	strcat(info, lin);
	sprintf(lin, "\n - Pilot value for next block : $%02X", hd[NEXTPILOTOFFSET]);
	strcat(info, lin);

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

	/* Move past the dummy count byte (successfully read during the search stage) */
	i++;
	dummy_cnt = blk[row]->xi;
	sprintf(lin, "\n - Dummy bytes : %d", dummy_cnt);
	strcat(info, lin);

	/* Read all dummy bytes */
	for (j = 0; j < dummy_cnt; i++, j++) {
		b = readttbyte(s + (i * BITSINABYTE), lp, sp, tp, en);

		if (b == -1) {
			rd_err++;

			/* for experts only */
			sprintf(lin, "\n - Read Error on dummy byte @$%X", s + (i * BITSINABYTE));
			strcat(info, lin);
		}
	}

	/* Read checkbyte */
	b = readttbyte(s + (i * BITSINABYTE), lp, sp, tp, en);
	if (b == -1) {
		/* Even if not within data, we cannot validate data reliably if
		   checksum is unreadable, so that increase read errors */
		rd_err++;

		/* for experts only */
		sprintf(lin, "\n - Read Error on checkbyte @$%X", s + (i * BITSINABYTE));
		strcat(info, lin);
	}

	blk[row]->cs_act = b  & 0xFF;

	/* Read execution address */
	i++;
	b = readttbyte(s + (i * BITSINABYTE), lp, sp, tp, en);
	if (b != -1) {
		exe_address = b;
	} else {
		/* Even if not within data, we cannot validate data reliably if
		   checksum is unreadable, so that increase read errors */
		rd_err++;
		exe_address = 0;

		/* for experts only */
		sprintf(lin, "\n - Read Error on LSB of execution address @$%X", s + (i * BITSINABYTE));
		strcat(info, lin);
	}
	i++;
	b = readttbyte(s + (i * BITSINABYTE), lp, sp, tp, en);
	if (b != -1) {
		exe_address |= (b << 8);
	} else {
		/* Even if not within data, we cannot validate data reliably if
		   checksum is unreadable, so that increase read errors */
		rd_err++;
		exe_address = 0;

		/* for experts only */
		sprintf(lin, "\n - Read Error on LSB of execution address @$%X", s + (i * BITSINABYTE));
		strcat(info, lin);
	}

	if (exe_address) {
		sprintf(lin, "\n - Exe Address : $%04X", exe_address);
		strcat(info, lin);
	} else {
		strcat(info, "\n - Exe Address : (corrupted)");
	}

	blk[row]->cs_exp = cb & 0xFF;
	blk[row]->rd_err = rd_err;

	return(rd_err);
}
