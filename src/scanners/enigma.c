/*
 * enigma.c (Enigma Variations - rewritten by Luigi Di Fraia, May 2014)
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
 * CBM inspection needed: Yes
 * Single on tape: Yes
 * Sync: Byte
 * Header: No
 * Data: Continuous
 * Checksum: No
 * Post-data: No
 * Trailer: Commonly no, but not necessarily (e.g. DNA Warrior)
 * Trailer homogeneous: No
 */

#include "../mydefs.h"
#include "../main.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define THISLOADER	ENIGMA

#define BITSINABYTE	8	/* a byte is made up of 8 bits here */

#define SYNCSEQSIZE	1	/* amount of sync bytes */
#define MAXTRAILER	8	/* max amount of trailer pulses read in */

#define MASTERLOADSIZE	0x200	/* size in bytes */

#ifdef _MSC_VER
#define inline __inline
#endif

static inline void get_enigma_addresses (int *buf, int bufsz, int blkindex, unsigned int *s, unsigned int *e)
{
	int seq_load_type1[15] = {
		/*
		 * Example from Defenders of the Earth:
		 * LDA #$00	; Load address LSB
		 * STA $60
		 * LDA #$E0	; MSB of the same
		 * STA $61
		 * LDA #$40	; End address LSB
		 * LDY #$FF	; MSB of the same
		 * JSR $02BA	; Load
		 */
		0xA9,XX,0x85,XX,0xA9,XX,0x85,XX,0xA9,XX,0xA0,XX,0x20,0xBA,0x02
	};
	int seq_load_type2[15] = {
		/*
		 * Example from Defenders of the Earth:
		 * LDA #$00	; Load address LSB
		 * LDX #$CC	; MSB of the same
		 * STA $60
		 * STX $61
		 * LDA #$00	; End address LSB
		 * LDY #$D0	; MSB of the same
		 * JSR $02BA	; Load
		 */
		0xA9,XX,0xA2,XX,0x85,XX,0x86,XX,0xA9,XX,0xA0,XX,0x20,0xBA,0x02
	};

	int index, offset1, offset2, minoffset, deltaoffset, sumoffsets;

	index = 1;
	minoffset = 0;
	deltaoffset = 0;
	sumoffsets = 0;

	do {
		sumoffsets += (minoffset + deltaoffset);

		offset1 = find_seq (buf + sumoffsets, bufsz, seq_load_type1, sizeof(seq_load_type1) / sizeof(seq_load_type1[0]));
		offset2 = find_seq (buf + sumoffsets, bufsz, seq_load_type2, sizeof(seq_load_type2) / sizeof(seq_load_type2[0]));

		if (offset1 < offset2 || offset2 == -1) {
			minoffset = offset1;
			deltaoffset = sizeof(seq_load_type1) / sizeof(seq_load_type1[0]);
		}
		if (offset2 < offset1 || offset1 == -1) {
			minoffset = offset2;
			deltaoffset = sizeof(seq_load_type2) / sizeof(seq_load_type2[0]);
		}

		index++;
	} while ((index <= blkindex) && !(offset1 == -1 && offset2 == -1));

	if (offset1 == -1 && offset2 == -1) {
		*s = 0;
		*e = 0;
	} else {
		*s  = buf[sumoffsets + minoffset + 1];

		if (minoffset == offset1) {
			*s |= buf[sumoffsets + minoffset + 5] << 8;
		} else {
			*s |= buf[sumoffsets + minoffset + 3] << 8;
		}
		*e  = buf[sumoffsets + minoffset + 9];
		*e |= buf[sumoffsets + minoffset + 11] << 8;
	}
}

void enigma_search(void)
{
	int i, h;			/* counters */
	int sof, sod, eod, eof, eop;	/* file offsets */
	int ib;				/* condition variable */

	int en, tp, sp, lp, sv;		/* encoding parameters */

	unsigned int s, e;		/* block locations referred to C64 memory */
	unsigned int x; 		/* block size */

	int enigma_index;		/* Index of file (0 = master loader) */

	int xinfo;			/* extra info used in addblockdef() */

	int masterloader[MASTERLOADSIZE];	/* Buffer for master loader (first 0x0200-byte turbo file) */


	en = ft[THISLOADER].en;
	tp = ft[THISLOADER].tp;
	sp = ft[THISLOADER].sp;
	lp = ft[THISLOADER].lp;
	sv = ft[THISLOADER].sv;

	if (!quiet)
		msgout("  Enigma Variations Tape");

	enigma_index = 0;

	/*
	 * First we retrieve the first file variables from the CBM data.
	 * We use CBM DATA index # 1 as we assume the tape image contains
	 * a single game.
	 * For compilations we should search and find the relevant file
	 * using the search code found e.g. in Biturbo.
	 *
	 * Note: if we can't get these variables we still fall back to the
	 *       "brute read" method.
	 */

	s = e = 0;	/* Assume we were unable to extract any info */

	ib = find_decode_block(CBM_DATA, 1);
	if (ib != -1) {
		int *buf, bufsz;

		bufsz = blk[ib]->cx;

		buf = (int *) malloc (bufsz * sizeof(int));
		if (buf != NULL) {
			/* Example from Defenders of teh Earth:
			 * JSR $03C4
			 * LDA #$00	; First file load address LSB
			 * STA $60
			 * LDY #$C0	; MSB of the same
			 * STY $61
			 *
			 * Note: the search pattern covers Implosion too */
			int seq_load_addr_first[10] = {
				0x20, 0xC4, 0x03, 0xA9, XX, 0x85, XX, 0xA0, XX, 0x84
			};

			int index, offset;

			/* Make an 'int' copy for use in find_seq() */
			for (index = 0; index < bufsz; index++)
				buf[index] = blk[ib]->dd[index];

			offset = find_seq (buf, bufsz, seq_load_addr_first, sizeof(seq_load_addr_first) / sizeof(seq_load_addr_first[0]));
			if (offset != -1) {
				/* Update s and e for the first block if info was found */
				s  = buf[offset + 4];
				s |= buf[offset + 8] << 8;
				e = s + 0x200;
			}

			free (buf);
		}
	}

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

			if (s == 0) {
				int b;

				/* scan through all readable bytes... */
				do {
					b = readttbyte(i, lp, sp, tp, en);
					if (b != -1)
						i += BITSINABYTE;
				} while (b != -1);

				/* Point to the first pulse of the last data byte (that's final) */
				eod = i - BITSINABYTE;

				/* Point to the last pulse of the last byte */
				eof = eod + BITSINABYTE - 1;

				if (addblockdef(THISLOADER, sof, sod, eod, eof, 0) >= 0)
					i = eof;	/* Search for further files starting from the end of this one */
			} else {
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

				/* Point to the first pulse of the last data byte (that's final) */
				eod = sod + (x - 1) * BITSINABYTE;

				/* Point to the last pulse of the last byte */
				eof = eod + BITSINABYTE - 1;

				/* Trace 'eof' to end of trailer (any value, both bit 1 and bit 0 pulses) */
				/* Note: only seen in DNA Warrior */
				h = 0;
				while (eof < tap.len - 1 &&
						h++ < MAXTRAILER &&
						readttbit(eof + 1, lp, sp, tp) >= 0)
					eof++;

				/* Store the info read from previous part as extra-info */
				xinfo = s + (e << 16);

				if (addblockdef(THISLOADER, sof, sod, eod, eof, xinfo) >= 0) {
					i = eof;	/* Search for further files starting from the end of this one */

					if (enigma_index == 0) {
						int j, b, rd_err;

						rd_err = 0;

						/* Store master loader */
						for (j = 0; j < MASTERLOADSIZE; j++) {
							b = readttbyte(sod + (j  * BITSINABYTE), lp, sp, tp, en);
							if (b == -1)
								rd_err++; /* Don't break here: during debug we will see how many errors occur */
							else
								masterloader[j] = b;
						}

						if (rd_err == 0) {
							enigma_index++;
							get_enigma_addresses (masterloader, MASTERLOADSIZE, enigma_index, &s, &e);
						} else {
							enigma_index = -1;
							s = e = 0;	/* We can't decode in case of error */
						}
					} else if (enigma_index > 0) {
						enigma_index++;
						get_enigma_addresses (masterloader, MASTERLOADSIZE, enigma_index, &s, &e);
					}
				}
			}
		}
	}
}


int enigma_describe(int row)
{
	int i, s;
	int en, tp, sp, lp;

	int b, rd_err;


	en = ft[THISLOADER].en;
	tp = ft[THISLOADER].tp;
	sp = ft[THISLOADER].sp;
	lp = ft[THISLOADER].lp;

	if (blk[row]->xi != 0) {
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

	} else {
		/* compute data size... */
		blk[row]->cx = ((blk[row]->p3 - blk[row]->p2) >> 3) + 1;
		blk[row]->cs = 0;			/* fake start address */
		blk[row]->ce = blk[row]->cx - 1;	/* fake end   address */

		/* get pilot & trailer length */
		blk[row]->pilot_len = (blk[row]->p2 - blk[row]->p1 - 8) >> 3;
		blk[row]->trail_len = 0;
	}

	/* Extract data */
	rd_err = 0;

	s = blk[row]->p2;

	if (blk[row]->dd != NULL)
		free(blk[row]->dd);

	blk[row]->dd = (unsigned char*)malloc(blk[row]->cx);

	for (i = 0; i < blk[row]->cx; i++) {
		b = readttbyte(s + (i * BITSINABYTE), lp, sp, tp, en);

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

	blk[row]->rd_err = rd_err;

	return(rd_err);
}

