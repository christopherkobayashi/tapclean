/*
 * turrican.c (rewritten by Luigi Di Fraia, Nov 2016 - armaeth@libero.it)
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
 * Header: Yes (in its own file)
 * Data: Continuous
 * Checksum: Yes
 * Post-data: No
 * Trailer: Only for Data block
 * Trailer homogeneous: Yes (bit 0 pulses)
 */

#include "../mydefs.h"
#include "../main.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define THISLOADER	TURR_HEAD

#define BITSINABYTE	8	/* a byte is made up of 8 bits here */

#define SYNCSEQSIZE	12	/* amount of sync bytes */
#define MAXTRAILER	2040	/* max amount of trailer pulses read in (after Data only) */

#define HEADERSIZE	0xC0	/* size of block header */
#define NAMESIZE	0x20	/* size of the filename portion */

#define LOADOFFSETH	1	/* load location (MSB) offset inside header */
#define LOADOFFSETL	0	/* load location (LSB) offset inside header */
#define ENDOFFSETH	3	/* end location (MSB) offset inside header */
#define ENDOFFSETL	2	/* end location (LSB) offset inside header */
#define NAMEOFFSET	5	/* filename offset inside header */

enum {
	FILE_TYPE_DATA = 0x00,
	FILE_TYPE_HEADER_RELOC = 0x01,
	FILE_TYPE_HEADER_NON_RELOC = 0x02
};

enum {
	STATE_SEARCH_HEADER = 0,
	STATE_SEARCH_DATA
};

/* If defined header contents are extracted to file and consequently the CRC32 is calculated */
#define TURRICAN_EXTRACT_HEADER

void turrican_search(void)
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
		0x0C, 0x0B, 0x0A, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01
	};
	int match;

	int ftype;			/* type of file */

	int xinfo;			/* extra info used in addblockdef() */

	int state;			/* whether to search for header or data */


	en = ft[THISLOADER].en;
	tp = ft[THISLOADER].tp;
	sp = ft[THISLOADER].sp;
	lp = ft[THISLOADER].lp;

	if (!quiet)
		msgout("  Turrican loader");

	state = STATE_SEARCH_HEADER;	/* Initially search for a header */

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

			ftype = readttbyte(sod, lp, sp, tp, en);

			switch (ftype) {
				case FILE_TYPE_HEADER_RELOC:		/* Header files are treated the same way by the original */
				case FILE_TYPE_HEADER_NON_RELOC:	/* code regardless of being relocatable or not */
					if (state != STATE_SEARCH_HEADER)
						continue;

					/* Read header */
					for (h = 0; h < HEADERSIZE; h++) {
						hd[h] = readttbyte(sod + (1 + h) * BITSINABYTE, lp, sp, tp, en);
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

					/* Store the info read from header as extra-info */
					xinfo = s + (e << 16);

					/* Point to the first pulse of the last header byte (that's final) */
					eod = sod + (1 + HEADERSIZE) * BITSINABYTE;

					/* Point to the last pulse of the last header byte */
					eof = eod + BITSINABYTE - 1;

					if (addblockdef(TURR_HEAD, sof, sod, eod, eof, xinfo) >= 0) {
						state = STATE_SEARCH_DATA;
						i = eof;	/* Search for further files starting from the end of this one */
					}

					break;

				case FILE_TYPE_DATA:	/* Data */
					if (state != STATE_SEARCH_DATA)
						continue;

					/* Point to the first pulse of the checkbyte (that's final) */
					eod = sod + (1 + x) * BITSINABYTE;

					/* Initially point to the last pulse of the checkbyte */
					eof = eod + BITSINABYTE - 1;

					/* Trace 'eof' to end of trailer (any value, both bit 1 and bit 0 pulses) */
					h = 0;
					while (eof < tap.len - 1 &&
							h++ < MAXTRAILER &&
							readttbit(eof + 1, lp, sp, tp) >= 0)
						eof++;

					if (addblockdef(TURR_DATA, sof, sod, eod, eof, xinfo) >= 0)
						i = eof;	/* Search for further files starting from the end of this one */

					/* Back to searching for header even if we failed adding block here */
					state = STATE_SEARCH_HEADER;

					break;

				default:
					continue;
			}
		} else {
			if (eop < 0)
				i = (-eop);
		}
	}
}

int turrican_describe(int row)
{
	int i, s;
	int ftype, lt;
	int en, tp, sp, lp;

	int b, rd_err;


	en = ft[THISLOADER].en;
	tp = ft[THISLOADER].tp;
	sp = ft[THISLOADER].sp;
	lp = ft[THISLOADER].lp;

	/* Note: addblockdef() is the glue between ft[] and blk[], so we can now read from blk[] */
	s = blk[row] -> p2;

	/* Read file type */
	ftype = readttbyte(s, lp, sp, tp, en);
	sprintf(lin,"\n - FILE type : $%02X", ftype);
	strcat(info, lin);

	/* Move past the file type */
	s += BITSINABYTE;

	lt = blk[row]->lt;
	if (lt == TURR_HEAD) {
		int dfs, dfe;
		int hd[HEADERSIZE];
		char bfname[NAMESIZE + 1], bfnameASCII[NAMESIZE + 1];

		/* Read header (it's safe to read it here for it was already decoded during the search stage) */
		for (i = 0; i < HEADERSIZE; i++)
			hd[i]= readttbyte(s + i * BITSINABYTE, lp, sp, tp, en);

		/* Read filename */
		for (i = 0; i < NAMESIZE; i++)
			bfname[i] = hd[NAMEOFFSET + i];
		bfname[i] = '\0';

		trim_string(bfname);
		pet2text(bfnameASCII, bfname);

		if (blk[row]->fn != NULL)
			free(blk[row]->fn);
		blk[row]->fn = (char*)malloc(strlen(bfnameASCII) + 1);
		strcpy(blk[row]->fn, bfnameASCII);

		/* Set load and end locations and size */
		blk[row]->cs = 0x0100;
		blk[row]->ce = 0x01BF;
		blk[row]->cx = blk[row]->ce - blk[row]->cs + 1;

		/* Retrieve C64 memory location for data load/end address from extra-info */
		dfs = blk[row]->xi & 0xFFFF;
		dfe = (blk[row]->xi & 0xFFFF0000) >> 16;

		sprintf(lin,"\n - DATA FILE Load address : $%04X", dfs);
		strcat(info,lin);
		sprintf(lin,"\n - DATA FILE End address : $%04X", dfe);
		strcat(info,lin);

		/* Read header contents */
		rd_err = 0;

#ifdef TURRICAN_EXTRACT_HEADER
		if (blk[row]->dd != NULL)
			free(blk[row]->dd);

		blk[row]->dd = (unsigned char*)malloc(blk[row]->cx);
#endif

		for (i = 0; i < blk[row]->cx; i++) {
			b = readttbyte(s + (i * BITSINABYTE), lp, sp, tp, en);

			if (b != -1) {
#ifdef TURRICAN_EXTRACT_HEADER
				blk[row]->dd[i] = b;
#endif
			} else {
#ifdef TURRICAN_EXTRACT_HEADER
				blk[row]->dd[i] = 0x69;  /* error code */
#endif
				rd_err++;

				/* for experts only */
#ifdef TURRICAN_EXTRACT_HEADER
				sprintf(lin, "\n - Read Error on byte @$%X (prg data offset: $%04X)", s + (i * BITSINABYTE), i + 2);
#else
				sprintf(lin, "\n - Read Error on byte @$%X (header payload offset: $%04X)", s + (i * BITSINABYTE), i);
#endif
				strcat(info, lin);
			}
		}
	} else {
		int cb;

		/* Retrieve C64 memory location for data load/end address from extra-info */
		blk[row]->cs = blk[row]->xi & 0xFFFF;
		blk[row]->ce = (blk[row]->xi & 0xFFFF0000) >> 16;
		blk[row]->cx = (blk[row]->ce - blk[row]->cs) + 1;

		/* Extract data and test checksum... */
		rd_err = 0;
		cb = 0;

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
	}

	/* Compute pilot & trailer lengths */

	/* pilot is in bytes... */
	blk[row]->pilot_len = (blk[row]->p2 - blk[row]->p1) / BITSINABYTE;

	/* ... trailer in pulses */
	blk[row]->trail_len = blk[row]->p4 - blk[row]->p3 - (BITSINABYTE - 1);

	/* if there IS pilot then disclude the sync sequence */
	if (blk[row]->pilot_len > 0)
		blk[row]->pilot_len -= SYNCSEQSIZE;

	return(rd_err);
}
