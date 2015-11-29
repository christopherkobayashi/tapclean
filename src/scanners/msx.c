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

enum {
	FILE_TYPE_D0_MEM_DUMP = 0xd0,
	FILE_TYPE_D3_BASIC = 0xd3,
	FILE_TYPE_EA_DATA = 0xea
};

enum {
	STATE_SEARCH_HEADER = 0,
	STATE_SEARCH_DATA
};

/* Header file's header info */
#define H_HEADERSIZE	16	/* size of header file header */
#define H_NAMESIZE	6	/* size of filename */
#define H_NAMEOFFSET	10	/* filename offset inside header */

/* Data file's header info for type 0xd0 */
#define D0_HEADERSIZE	6	/* size of data file header */
#define D0_LOADOFFSETH	1	/* load location (MSB) offset inside header */
#define D0_LOADOFFSETL	0	/* load location (LSB) offset inside header */
#define D0_ENDOFFSETH	3	/* end  location (MSB) offset inside header */
#define D0_ENDOFFSETL	2	/* end  location (LSB) offset inside header */
#define D0_EXECOFFSETH	5	/* start location (MSB) offset inside header */
#define D0_EXECOFFSETL	4	/* start location (LSB) offset inside header */

/* Data file's sentinel for type 0xd3 */
#define D3_SENTINELSIZE	10

/* Data file's payload size for type 0xea */
#define EA_BLOCKSIZE	256
#define EA_EOF		0x1a

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
	int sn[D3_SENTINELSIZE];	/* buffer to store sentinel for BASIC file */
	int dh[D0_HEADERSIZE];		/* buffer to store dump data header info */
	int b, b1;			/* byte values */

	int en, tp, sp, lp, sv;		/* encoding parameters */

	unsigned int s, e;		/* block locations referred to C64 memory */
	unsigned int x; 		/* block size */

	int ftype;			/* type of file */

	int xinfo;			/* extra info used in addblockdef() */

	int state;			/* whether to search for header or data */


	en = ft[MSX_HEAD].en;
	tp = ft[MSX_HEAD].tp;
	sp = ft[MSX_HEAD].sp;
	lp = ft[MSX_HEAD].lp;
	sv = ft[MSX_HEAD].sv;

	if (!quiet)
		msgout("  MSX tape");

	state = STATE_SEARCH_HEADER;	/* Initially search for a header */

	for (i = 20; i > 0 && i < tap.len - BITSINABYTE; i++) {
		if (state == STATE_SEARCH_HEADER) {
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
					if (h < H_NAMEOFFSET && 
							hd[h] != FILE_TYPE_D3_BASIC && 
							hd[h] != FILE_TYPE_D0_MEM_DUMP && 
							hd[h] != FILE_TYPE_EA_DATA)
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

					ftype = hd[0];

					/* Search for the corresponding data file */
					state = STATE_SEARCH_DATA;
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

				switch (ftype) {
					case FILE_TYPE_D3_BASIC:
						for (h = 0; h < D3_SENTINELSIZE; h++)
							sn[h] = -1;

						for (h = 0, pcount = 0;; h++) {
							b = msx_read_byte(sod + pcount, MSX_DATA);
							if (b == -1)
								break;
							pcount += b >> 8;
							//printf("%c", b & 0xff);
							sn[h % D3_SENTINELSIZE] = b & 0xff;
							b1 = b;
						}
						if (h < 10) {
							/* Back to searching for header then */
							state = STATE_SEARCH_HEADER;
							continue;
						}

						/* Set size */
						x = h;

						/* Verify we found a 10-zero sentinel at the end */
						for (h = 0; h < D3_SENTINELSIZE; h++)
							if (sn[h] != 0x00)
								break;
						if (h != D3_SENTINELSIZE) {
							/* Back to searching for header then */
							state = STATE_SEARCH_HEADER;
							continue;
						}

						/* Point to the first pulse of the last byte (that's final) */
						eod = sod + pcount - (b1 >> 8);

						/* Point to the last pulse of the last byte */
						eof = eod + (b1 >> 8) - 1;

						xinfo = x;

						if (addblockdefex(MSX_DATA, sof, sod, eod, eof, xinfo, ftype) >= 0)
							i = eof;	/* Search for further files starting from the end of this one */

						/* Back to searching for header even if we failed adding block here */
						state = STATE_SEARCH_HEADER;

						break;

					case FILE_TYPE_D0_MEM_DUMP:
						/* Read data header header */
						for (h = 0, pcount = 0; h < D0_HEADERSIZE; h++) {
							b = msx_read_byte(sod + pcount, MSX_DATA);
							if (b == -1)
								break;
							pcount += b >> 8;
							dh[h] = b & 0xff;
						}
						if (h != D0_HEADERSIZE) {
							/* Back to searching for header then */
							state = STATE_SEARCH_HEADER;
							continue;
						}

						/* Extract load and end locations */
						s = dh[D0_LOADOFFSETL] + (dh[D0_LOADOFFSETH] << 8);
						e = dh[D0_ENDOFFSETL]  + (dh[D0_ENDOFFSETH]  << 8);

						/* Prevent int wraparound when subtracting 1 from end location
						   to get the location of the last loaded byte */
						if (e == 0)
							e = 0xFFFF;
						else
							e--;

						/* Plausibility check */
						if (e < s) {
							/* Back to searching for header then */
							state = STATE_SEARCH_HEADER;
							continue;
						}

						/* Compute size */
						x = e - s + 1;

						/* Parse all data in file in order to point to its end */
						for (h = 0; h < x; h++) {
							b = msx_read_byte(sod + pcount, MSX_DATA);
							/* For now we don't try to find the next start bit in order to continue reading */
							if (b == -1)
								break;
							pcount += b >> 8;
							b1 = b;
						}

						/* Point to the first pulse of the last byte (that's final) */
						eod = sod + pcount - (b1 >> 8);

						/* Point to the last pulse of the last byte */
						eof = eod + (b1 >> 8) - 1;

						/* Extra byte after data (its meaning is yet unknown) */
						b = msx_read_byte(sod + pcount, MSX_DATA);
						if (b != -1)
							eof += b >> 8;

						/* Store the info read from header as extra-info so we don't need to extract it again at the describe stage */
						xinfo = s + (e << 16);

						if (addblockdefex(MSX_DATA, sof, sod, eod, eof, xinfo, ftype) >= 0)
							i = eof;

						/* Back to searching for header even if we failed adding block here */
						state = STATE_SEARCH_HEADER;

						break;

					case FILE_TYPE_EA_DATA:
						/* Parse all data in file in order to point to its end */
						for (h = 0, pcount = 0; h < EA_BLOCKSIZE; h++) {
							b = msx_read_byte(sod + pcount, MSX_DATA);
							if (b == -1)
								break;
							pcount += b >> 8;
							b1 = b;

							/* Verify if there is at least an EOF char */
							if ((b & 0xff) == EA_EOF)
								state = STATE_SEARCH_HEADER;
						}
						if (h != EA_BLOCKSIZE) {
							/* Back to searching for header then */
							state = STATE_SEARCH_HEADER;
							continue;
						}

						/* Set size */
						x = h;

						/* Point to the first pulse of the last byte (that's final) */
						eod = sod + pcount - (b1 >> 8);

						/* Point to the last pulse of the last byte */
						eof = eod + (b1 >> 8) - 1;

						xinfo = x;

						if (addblockdefex(MSX_DATA, sof, sod, eod, eof, xinfo, ftype) >= 0)
							i = eof;	/* Search for further files starting from the end of this one */
						else
							state = STATE_SEARCH_HEADER;

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
	int dh[D0_HEADERSIZE];
	int en, tp, sp, lp;
	char bfname[H_NAMESIZE + 1], bfnameASCII[H_NAMESIZE + 1];
	int b, b1;

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

		sprintf(lin,"\n - DATA FILE type : $%02X", hd[0]);
		strcat(info, lin);
		switch(hd[0]) {
			case FILE_TYPE_D3_BASIC:
				strcat(info, " - BASIC File (CSAVE/CLOAD)");
				break;
			case FILE_TYPE_D0_MEM_DUMP:
				strcat(info, " - Memory Dump (BSAVE/BLOAD)");
				break;
			case FILE_TYPE_EA_DATA:
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

		switch (blk[row]->meta1) {
			case FILE_TYPE_D3_BASIC:
				blk[row]->cx = blk[row]->xi;

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

				/* Compute pilot & trailer lengths */

				/* pilot is in pulses... */
				blk[row]->pilot_len = blk[row]->p2 - blk[row]->p1;

				/* ... trailer in pulses */
				blk[row]->trail_len = blk[row]->p4 - blk[row]->p3 - ((b >> 8) - 1);

				break;

			case FILE_TYPE_D0_MEM_DUMP:
				/* Retrieve memory location for load/end address from extra-info */
				blk[row]->cs = blk[row]->xi & 0xFFFF;
				blk[row]->ce = (blk[row]->xi & 0xFFFF0000) >> 16;
				blk[row]->cx = (blk[row]->ce - blk[row]->cs) + 1;

				/* Safe to read header here as we already read data at search time */
				for (i = 0, pcount = 0; i < D0_HEADERSIZE; i++) {
					b = msx_read_byte(s + pcount, MSX_HEAD);
					pcount += b >> 8;
					dh[i] = b & 0xff;
				}

				sprintf(lin, "\n - Execution address: $%04X", dh[D0_EXECOFFSETL] + (dh[D0_EXECOFFSETH] << 8));
				strcat(info, lin);

				if (blk[row]->dd != NULL)
					free(blk[row]->dd);

				blk[row]->dd = (unsigned char*)malloc(blk[row]->cx);

				for (i = 0; i < blk[row]->cx; i++) {
					b = msx_read_byte(s + pcount, MSX_HEAD);
					if (b != -1) {
						blk[row]->dd[i] = b;
						pcount += b >> 8;
						b1 = b;
					} else {
						blk[row]->dd[i] = 0x69;  /* error code */
						rd_err++;

						/* for experts only */
						sprintf(lin, "\n - Read Error on byte @$%X (prg data offset: $%04X)", s + (i * BITSINABYTE), i + 2);
						strcat(info, lin);

						/* For now we don't try to find the next start bit in order to continue reading */
						break;
					}
				}

				b = msx_read_byte(s + pcount, MSX_HEAD);
				if (b != -1) {
					sprintf(lin, "\n - Post data byte: $%02X", b & 0xff);
					strcat(info, lin);
				}

				/* Compute pilot & trailer lengths */

				/* pilot is in pulses... */
				blk[row]->pilot_len = blk[row]->p2 - blk[row]->p1;

				/* ... trailer in pulses */
				blk[row]->trail_len = blk[row]->p4 - blk[row]->p3 - ((b1 >> 8) - 1);

				break;

			case FILE_TYPE_EA_DATA:
				blk[row]->cx = blk[row]->xi;

				blk[row]->cs = 0x0000;
				blk[row]->ce = blk[row]->cs + blk[row]->cx - 1;

				if (blk[row]->dd != NULL)
					free(blk[row]->dd);

				blk[row]->dd = (unsigned char*)malloc(blk[row]->cx);

				for (i = 0, pcount = 0; i < blk[row]->cx; i++) {
					b = msx_read_byte(s + pcount, MSX_HEAD);
					if (b != -1) {
						blk[row]->dd[i] = b;
						pcount += b >> 8;
						b1 = b;
					} else {
						blk[row]->dd[i] = 0x69;  /* error code */
						rd_err++;

						/* for experts only */
						sprintf(lin, "\n - Read Error on byte @$%X (prg data offset: $%04X)", s + (i * BITSINABYTE), i + 2);
						strcat(info, lin);

						/* For now we don't try to find the next start bit in order to continue reading */
						break;
					}
				}

				/* Compute pilot & trailer lengths */

				/* pilot is in pulses... */
				blk[row]->pilot_len = blk[row]->p2 - blk[row]->p1;

				/* ... trailer in pulses */
				blk[row]->trail_len = blk[row]->p4 - blk[row]->p3 - ((b1 >> 8) - 1);

				break;
		}
	}

	blk[row]->rd_err = rd_err;

	return rd_err;
}
