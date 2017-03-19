/*
 * powerload.c (by Luigi Di Fraia, May 2011)
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
 * Single on tape: Commonly yes, but not necessarily (e.g. Rocket Roger)
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define THISLOADER	POWERLOAD

#define BITSINABYTE	8	/* a byte is made up of 8 bits here */

#define SYNCSEQSIZE	10	/* amount of sync bytes */
#define MAXTRAILER	2048	/* max amount of trailer pulses read in */
#define MAXPOSTDATASIZE	(BITSINABYTE * 0x01D0)	/* Max amount of post-data pulses */

#define LOADOFFSETH	0x03	/* load location (MSB) offset inside CBM data */
#define LOADOFFSETL	0x01	/* load location (LSB) offset inside CBM data */
#define ENDOFFSETH	0x0B	/* end location (MSB) offset inside CBM data */
#define ENDOFFSETL	0x09	/* end location (LSB) offset inside CBM data */
#define EXECOFFSETL	0xC0	/* execution address (LSB) offset inside CBM data */
#define EXECOFFSETH	0xC1	/* execution address (MSB) offset inside CBM data */

void powerload_search (void)
{
	int i, h, post;			/* counters */
	int sof, sod, eod, eof, eop;	/* file offsets */
	int pat[SYNCSEQSIZE];		/* buffer to store a sync train */
	int ib;				/* condition variable */

	int en, tp, sp, lp;		/* encoding parameters */

	unsigned int s, e;		/* block locations referred to C64 memory */
	unsigned int x; 		/* block size */

	/* Expected sync pattern */
	static int sypat[SYNCSEQSIZE] = {
		0x09,0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00
	};
	int match;			/* condition variable */

	int xinfo, meta1;		/* extra info used in addblockdef() */


	en = ft[THISLOADER].en;
	tp = ft[THISLOADER].tp;
	sp = ft[THISLOADER].sp;
	lp = ft[THISLOADER].lp;

	if (!quiet)
		msgout("  Power Load");

	/*
	 * First we retrieve the Power Load variables from the CBM data.
	 * We use CBM DATA index # 3 as we assume the tape image contains
	 * a single game.
	 * For compilations we should search and find the relevant file
	 * using the search code found e.g. in Biturbo.
	 */
	ib = find_decode_block(CBM_DATA, 3);
	if (ib == -1)
		return;		/* failed to locate CBM data. */

	/* Percy Pigeon has an extra CBM file before the main loader */
	if (blk[ib]->cx == 0x83) {
		ib = find_decode_block(CBM_DATA, 5);
		if (ib == -1)
			return;		/* failed to locate CBM data. */
	}

	/* Basic validation before accessing array elements */
	if (blk[ib]->cx < ENDOFFSETL + 1)
		return;

	s = blk[ib]->dd[LOADOFFSETL] + (blk[ib]->dd[LOADOFFSETH] << 8);
	e = blk[ib]->dd[ENDOFFSETL] + (blk[ib]->dd[ENDOFFSETH] << 8);

	/* Save the real end address for later display */
	meta1 = e << 16;

	/* Look for a jump to the execution address (not mandatory) */
	if (blk[ib]->cx >= EXECOFFSETH + 1 && blk[ib]->dd[EXECOFFSETL-1] == 0x4C)
		meta1 |= blk[ib]->dd[EXECOFFSETL] + (blk[ib]->dd[EXECOFFSETH] << 8);

	/* Prevent int wraparound when subtracting 1 from end location
	   to get the location of the last loaded byte */
	if (e == 0)
		e = 0xFFFF;
	else
		e--;

	/* Plausibility check */
	if (e < s)
		return;

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

			/* Trace back to find the checkbyte position */
			post = eof - (eof - eod) % BITSINABYTE;	/* Byte boundary */

			while (post > eod + BITSINABYTE &&
					post > eof - MAXTRAILER + BITSINABYTE &&
					readttbyte(post - BITSINABYTE, lp, sp, tp, en) == 0)
				post -= BITSINABYTE;

			/*
			 * Extend data block to include post-data (Note: eod
			 * points to the first pulse of last data byte and we
			 * need to exclude the checksum too, hence "- 2")
			 */
			if (post - eod > 2 * BITSINABYTE)
				e = (e + (post - eod) / BITSINABYTE - 2) & 0xFFFF;

			/* Plausibility check */
			if (e < s)
				continue;

			/* Extend block info too (point to first pulse of the checkbyte) */
			eod = post - BITSINABYTE;

			/* Store the info read from CBM part as extra-info */
			xinfo = s + (e << 16);

			if (addblockdefex(THISLOADER, sof, sod, eod, eof, xinfo, meta1) >= 0) {
				i = eof;	/* Search for further files starting from the end of this one */

				/* Non-standard Power Loader (Rocket Roger loads an extra turbo block) */
				if ((meta1 & 0xFFFF) == 0) {
					int *buf, bufsz;

					bufsz = blk[ib]->cx;

					buf = (int *) malloc (bufsz * sizeof(int));
					if (buf != NULL) {
						int seq_load_lsb[] = {0xA9, XX, 0x8D, 0x48, 0xCE};
						int seq_load_msb[] = {0xA9, XX, 0x8D, 0x4A, 0xCE};
						int seq_end_lsb[] = {0xA9, XX, 0x8D, 0x50, 0xCE};
						int seq_end_msb[] = {0xA9, XX, 0x8D, 0x52, 0xCE};
						int seq_exec[] = {0x09, 0x20, 0x85, 0x01, 0x4C, XX, XX};

						int index, offset;

						unsigned int next_s, next_e, next_meta1;

						/* Make an 'int' copy for use in find_seq() */
						for (index = 0; index < bufsz; index++)
							buf[index] = blk[ib]->dd[index];

						index = 0;

						offset = find_seq (buf, bufsz, seq_load_lsb, sizeof(seq_load_lsb) / sizeof(seq_load_lsb[0]));
						if (offset != -1) {
							index |= 1;
							next_s = buf[offset + 1];
						}

						offset = find_seq (buf, bufsz, seq_load_msb, sizeof(seq_load_msb) / sizeof(seq_load_msb[0]));
						if (offset != -1) {
							index |= 2;
							next_s |= buf[offset + 1] << 8;
						}

						offset = find_seq (buf, bufsz, seq_end_lsb, sizeof(seq_end_lsb) / sizeof(seq_end_lsb[0]));
						if (offset != -1) {
							index |= 4;
							next_e = buf[offset + 1];
						}

						offset = find_seq (buf, bufsz, seq_end_msb, sizeof(seq_end_msb) / sizeof(seq_end_msb[0]));
						if (offset != -1) {
							index |= 8;
							next_e |= buf[offset + 1] << 8;
						}

						offset = find_seq (buf, bufsz, seq_exec, sizeof(seq_exec) / sizeof(seq_exec[0]));
						if (offset != -1) {
							index |= 16;
							next_meta1 = (next_e << 16) | buf[offset + 5] | (buf[offset + 6] << 8);
						}

						/* Update s and e for next block if info was found */
						if ((index & 15) == 15) {
							s = next_s;
							e = next_e;
						}

						/* Set execution address for next block if info was found */
						if ((index & 16) == 16)
							meta1 = next_meta1;

						free (buf);
					}
				}
			}

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

	sprintf(lin, "\n - Real end address + 1 : $%04X", (blk[row]->meta1 >> 16) & 0xFFFF);
	strcat(info, lin);

	if ((blk[row]->meta1 & 0xFFFF) != 0) {
		sprintf(lin, "\n - Exe Address (in CBM data) : $%04X", blk[row]->meta1 & 0xFFFF);
		strcat(info, lin);
	}

	blk[row]->cs_exp = cb & 0xFF;
	blk[row]->cs_act = b  & 0xFF;
	blk[row]->rd_err = rd_err;

	return(rd_err);
}
