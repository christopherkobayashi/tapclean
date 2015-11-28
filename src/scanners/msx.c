/*
 * msx.c (by Luigi Di Fraia, Nov 2015 - armaeth@libero.it)
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
 * CBM inspection needed: N/A
 * Single on tape: No
 * Sync: N/A
 * Header: Yes
 * Data: Continuous
 * Checksum: No
 * Post-data: No
 * Trailer: No
 * Trailer homogeneous: N/A
 */

#include "../mydefs.h"
#include "../main.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define BITSINABYTE	11	/* a byte is made up of 11 bits here */

/* Header file's header info */
#define H_HEADERSIZE	16	/* size of header file header */
#define H_NAMESIZE	6	/* size of filename */
#define H_NAMEOFFSET	10	/* filename offset inside header */

/* Data file's header info for type 0xd0 */
#define D_HEADERSIZE	6	/* size of data file header */
#define D_LOADOFFSETH	1	/* load location (MSB) offset inside header */
#define D_LOADOFFSETL	0	/* load location (LSB) offset inside header */
#define D_ENDOFFSETH	3	/* end  location (MSB) offset inside header */
#define D_ENDOFFSETL	2	/* end  location (LSB) offset inside header */
#define D_EXECOFFSETH	5	/* start location (MSB) offset inside header */
#define D_EXECOFFSETL	4	/* start location (LSB) offset inside header */

/* Data file's sentinel for type 0xd3 */
#define D_SENTINELSIZE	10

int msx_read_byte (int pos, int lt)
{
	int i, bit_count, valid, pulse, bit, byte;

	//printf("\nmsx_read_byte @ %d", pos);
	i = 0;
	bit_count = 0;
	byte = 0x00;

	while (bit_count < BITSINABYTE) {
		if (pos + i < 20 || pos + i > tap.len - 1)
			return -1;

		if (is_pause_param(pos + i)) {
			add_read_error(pos + i);
			return -1;
		}

		valid = 0;
		pulse = tap.tmem[pos+i];
		if(pulse > (ft[lt].sp - tol) && pulse < (ft[lt].sp + tol)) {
			bit = 1;
			valid++;
			i += 2;
		}
		if(pulse > (ft[lt].lp - tol) && pulse < (ft[lt].lp + tol)) {
			bit = 0;
			valid++;
			i++;
		}
		if (!valid) {
			add_read_error(pos + i);
			return -1;
		}
		bit_count++;	/* Acknowledge bit */

		if (bit_count == 1 && bit != 0)		/* One start bit */
			return -1;
		if (bit_count >= 2 && bit_count <= 9)
			byte |= bit << (bit_count - 2);	/* LSbF */
		if (bit_count >= 10 && bit != 1)	/* Two stop bits */
			return -1;
	}

	return (i << 8) | byte;
}

int msx_find_pilot (int pos, int lt)
{
	int z, sp, lp, tp, pmin;

	sp = ft[lt].sp;
	lp = ft[lt].lp;
	pmin = ft[lt].pmin;

	tp = (sp + lp) / 2;

	if (readttbit(pos, lp, sp, tp) == 0) {
		z = 0;

		while (readttbit(pos, lp, sp, tp) == 0 && pos < tap.len) {
			z++;
			pos++;
		}

		if (z == 0)
			return 0;

		if (z < pmin)
			return -pos;

		if (z >= pmin)
			return pos;
	}

	return 0;
}

void msx_search (void)
{
	int i, h, pcount;		/* counters */
	int sof, sod, eod, eof, eop;	/* file offsets */
	int hd[H_HEADERSIZE];		/* buffer to store block header info */
	int sn[D_SENTINELSIZE];		/* buffer to store sentinel for BASIC file */
	int b, b1;			/* byte value */

	int en, tp, sp, lp, sv;		/* encoding parameters */

	unsigned int s, e;		/* block locations referred to C64 memory */
	unsigned int x; 		/* block size */

	int xinfo;			/* extra info used in addblockdef() */

	int state;			/* whether to search for header or data */


	en = ft[MSX_HEAD].en;
	tp = ft[MSX_HEAD].tp;
	sp = ft[MSX_HEAD].sp;
	lp = ft[MSX_HEAD].lp;
	sv = ft[MSX_HEAD].sv;

	if (!quiet)
		msgout("  MSX tape");

	state = 0;	/* Initially search for a header */

	for (i = 20; i > 0 && i < tap.len - BITSINABYTE; i++) {
		if (state == 0) {
			eop = msx_find_pilot(i, MSX_HEAD);

			if (eop > 0) {
				/* Valid pilot found, mark start of header file */
				sof = i;
				i = eop;

				/* Mark start of header contents */
				sod = i;

				/* Read header */
				for (h = 0, pcount = 0; h < H_HEADERSIZE; h++) {
					b = msx_read_byte(sod + pcount, MSX_HEAD);
					if (b == -1)
						break;
					hd[h] = b & 0xff;
					pcount += b >> 8;
					if (h < H_NAMEOFFSET && hd[h] != 0xd3 && hd[h] != 0xd0 && hd[h] != 0xea)
						break;
				}
				if (h != H_HEADERSIZE)
					continue;

				/* Point to the first pulse of the last file name byte (that's final) */
				eod = sod + pcount - (b >> 8);

				/* Point to the last pulse of the last byte */
				eof = eod + (b >> 8) - 1;

				if (addblockdef(MSX_HEAD, sof, sod, eod, eof, 0) >= 0) {
					i = eof;	/* Search for further files starting from the end of this one */
	
					/* For now support is only complete for file type 0xd3 */
					if (hd[0] == 0xd3 || hd[0] == 0xd0)
						state++;
				}
			} else {
				if (eop < 0) {
					i = (-eop);
				}
			}
		} else {
			eop = msx_find_pilot(i, MSX_DATA);
			
			if (eop > 0) {
				/* Valid pilot found, mark start of data file */
				sof = i;
				i = eop;

				/* Mark start of data */
				sod = i;

				switch(hd[0]) {
					case 0xd3:
						for (h = 0; h < D_SENTINELSIZE; h++)
							sn[h] = -1;

						for (h = 0, pcount = 0;; h++) {
							b = msx_read_byte(sod + pcount, MSX_DATA);
							if (b == -1)
								break;
							pcount += b >> 8;
							//printf("%c", b & 0xff);
							sn[h % D_SENTINELSIZE] = b & 0xff;
							b1 = b;
						}
						if (h < 10)
							continue;

						/* Set size */
						x = h;

						/* Verify we found a 10-zero sentinel at the end */
						for (h = 0; h < D_SENTINELSIZE; h++)
							if (sn[h] != 0x00)
								break;
						if (h != D_SENTINELSIZE)
							continue;

						/* Point to the first pulse of the last file name byte (that's final) */
						eod = sod + pcount - (b1 >> 8);

						/* Point to the last pulse of the last byte */
						eof = eod + (b1 >> 8) - 1;

						xinfo = hd[0] + (x << 16);

						if (addblockdef(MSX_DATA, sof, sod, eod, eof, xinfo) >= 0) {
							i = eof;	/* Search for further files starting from the end of this one */
							state--;
						}
						break;

					case 0xd0:
						/* Read header */
						for (h = 0, pcount = 0; h < H_HEADERSIZE; h++) {
							b = msx_read_byte(sod + pcount, MSX_DATA);
							if (b == -1)
								break;
							pcount += b >> 8;
							hd[h] = b & 0xff;
						}
						if (h != H_HEADERSIZE)
							continue;

						/* Extract load and end locations */
						s = hd[D_LOADOFFSETL] + (hd[D_LOADOFFSETH] << 8);
						e = hd[D_ENDOFFSETL]  + (hd[D_ENDOFFSETH]  << 8);

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

						/* Parse all data in file in order to point to its end */
						for (h = 0; h < x; h++) {
							b = msx_read_byte(sod + pcount, MSX_DATA);
							/* For now we don't try to find the next start bit and continue reading */
							if (b == -1)
								break;
							pcount += b >> 8;
							b1 = b;
						}

						/* Point to the first pulse of the last file name byte (that's final) */
						eod = sod + pcount - (b1 >> 8);

						/* Point to the last pulse of the last byte */
						eof = eod + (b1 >> 8) - 1;

						/* Store the info read from header as extra-info so we don't neext to extract it again */
						xinfo = s + (e << 16);

						if (addblockdef(MSX_DATA, sof, sod, eod, eof, xinfo) >= 0)
							i = eof;

						/* Back to searching for header even if we failed here */
						state--;

						break;

					case 0xea:
						state--;
						break;
				}
			} else {
				if (eop < 0) {
					i = (-eop);
				}
			}
		}
	}

}

int msx_describe (int row)
{
	int i, pcount;
	int hd[H_HEADERSIZE];
	int en, tp, sp, lp;
	char bfname[H_NAMESIZE + 1], bfnameASCII[H_NAMESIZE + 1];
	int b;

	int s;
	int rd_err;


	if (blk[row]->lt == MSX_HEAD) {
		en = ft[MSX_HEAD].en;
		tp = ft[MSX_HEAD].tp;
		sp = ft[MSX_HEAD].sp;
		lp = ft[MSX_HEAD].lp;

		/* Note: addblockdef() is the glue between ft[] and blk[], so we can now read from blk[] */
		s = blk[row] -> p2;

		/* Read header (it's safe to read it here for it was already decoded during the search stage) */
		for (i = 0, pcount = 0; i < H_HEADERSIZE; i++) {
			b = msx_read_byte(s + pcount, MSX_HEAD);
			hd[i]= b & 0xff;
			pcount += b >> 8;
		}

		/* Read filename */
		for (i = 0; i < H_NAMESIZE; i++)
			bfname[i] = hd[H_NAMEOFFSET + i];
		bfname[i] = '\0';
	
		trim_string(bfname);
		pet2text(bfnameASCII, bfname);

		if (blk[row]->fn != NULL)
			free(blk[row]->fn);
		blk[row]->fn = (char*)malloc(strlen(bfnameASCII) + 1);
		strcpy(blk[row]->fn, bfnameASCII);

		blk[row]->cs = 0;
		blk[row]->cx = 0;
		blk[row]->ce = 0;

		sprintf(lin,"\n - File Type : $%02X", hd[0]);
		strcat(info, lin);
		switch(hd[0]) {
			case 0xd3:
				strcat(info, " - BASIC File (CSAVE/CLOAD)");
				break;
			case 0xd0:
				strcat(info, " - Memory Dump (BSAVE/BLOAD)");
				break;
			case 0xea:
				strcat(info, " - Data File (SAVE/LOAD/OPEN)");
				break;
		}

		/* Compute pilot & trailer lengths */

		/* pilot is in pulses... */
		blk[row]->pilot_len = blk[row]->p2 - blk[row]->p1;

		/* ... trailer in pulses */
		blk[row]->trail_len = blk[row]->p4 - blk[row]->p3 - ((b >> 8) - 1);

		rd_err = 0;
	} else {
		en = ft[MSX_DATA].en;
		tp = ft[MSX_DATA].tp;
		sp = ft[MSX_DATA].sp;
		lp = ft[MSX_DATA].lp;

		/* Note: addblockdef() is the glue between ft[] and blk[], so we can now read from blk[] */
		s = blk[row] -> p2;

		/* Extract data */
		rd_err = 0;

		switch (blk[row]->xi & 0xff) {
			case 0xd3:
				blk[row]->cx = (blk[row]->xi & 0xFFFF0000) >> 16;

				blk[row]->cs = 0x8001;
				blk[row]->ce = blk[row]->cs + blk[row]->cx - 1;

				if (blk[row]->dd != NULL)
					free(blk[row]->dd);

				blk[row]->dd = (unsigned char*)malloc(blk[row]->cx);

				/* Safe to read here as we already read data at search time, including the sentinel */
				for (i = 0, pcount = 0; i < blk[row]->cx; i++) {
					b = msx_read_byte(s + pcount, MSX_HEAD);
					blk[row]->dd[i] = b;
					pcount += b >> 8;
				}
				break;

			case 0xd0:
				/* Retrieve memory location for load/end address from extra-info */
				blk[row]->cs = blk[row]->xi & 0xFFFF;
				blk[row]->ce = (blk[row]->xi & 0xFFFF0000) >> 16;
				blk[row]->cx = (blk[row]->ce - blk[row]->cs) + 1;

				if (blk[row]->dd != NULL)
					free(blk[row]->dd);

				blk[row]->dd = (unsigned char*)malloc(blk[row]->cx);

				for (i = 0, pcount = 0; i < blk[row]->cx; i++) {
					b = msx_read_byte(s + pcount, MSX_HEAD);
					if (b != -1) {
						blk[row]->dd[i] = b;
						pcount += b >> 8;
					} else {
						blk[row]->dd[i] = 0x69;  /* error code */
						rd_err++;

						/* for experts only */
						sprintf(lin, "\n - Read Error on byte @$%X (prg data offset: $%04X)", s + (i * BITSINABYTE), i + 2);
						strcat(info, lin);

						/* For now we don't try to find the next start bit and continue reading */
						break;
					}
				}
				break;
		}
	}

	blk[row]->rd_err = rd_err;

	return rd_err;
}
