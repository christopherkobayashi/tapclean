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

#define THISLOADER	MSX_HEAD

#define BITSINABYTE	11	/* a byte is made up of 11 bits here */

#define HEADERSIZE	16	/* size of block header */
#define NAMESIZE	6	/* size of filename */

#define NAMEOFFSET	10	/* filename offset inside header */

int msx_read_byte (int pos)
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
		if(pulse > (ft[THISLOADER].sp - tol) && pulse < (ft[THISLOADER].sp + tol)) {
			bit = 1;
			valid++;
			i += 2;
		}
		if(pulse > (ft[THISLOADER].lp - tol) && pulse < (ft[THISLOADER].lp + tol)) {
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

int msx_find_pilot (int pos)
{
	int z, sp, lp, tp, pmin;

	sp = ft[THISLOADER].sp;
	lp = ft[THISLOADER].lp;
	pmin = ft[THISLOADER].pmin;

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
	int i, h, pcount;			/* counters */
	int sof, sod, eod, eof, eop;	/* file offsets */
	int hd[HEADERSIZE];		/* buffer to store block header info */
	int b;				/* byte value */

	int en, tp, sp, lp, sv;		/* encoding parameters */

//	unsigned int s, e;		/* block locations referred to C64 memory */
//	unsigned int x; 		/* block size */


	en = ft[THISLOADER].en;
	tp = ft[THISLOADER].tp;
	sp = ft[THISLOADER].sp;
	lp = ft[THISLOADER].lp;
	sv = ft[THISLOADER].sv;

	if (!quiet)
		msgout("  MSX tape");

	for (i = 20; i > 0 && i < tap.len - BITSINABYTE; i++) {
		eop = msx_find_pilot(i);

		if (eop > 0) {
			/* Valid pilot found, mark start of file */
			sof = i;
			i = eop;

			/* Mark start of data */
			sod = i;

			/* Read header */
			for (h = 0, pcount = 0; h < HEADERSIZE; h++) {
				b = msx_read_byte(sod + pcount);
				if (b == -1)
					break;
				hd[h] = b & 0xff;
				pcount += b >> 8;
				if (h < NAMEOFFSET && hd[h] != 0xd3 && hd[h] != 0xd0 && hd[h] != 0xea)
					break;
			}
			if (h != HEADERSIZE)
				continue;

			/* Point to the first pulse of the last file name byte (that's final) */
			eod = sod + pcount - (b >> 8);

			/* Point to the last pulse of the last byte */
			eof = eod + (b >> 8) - 1;

			if (addblockdef(MSX_HEAD, sof, sod, eod, eof, 0) >= 0)
				i = eof;	/* Search for further files starting from the end of this one */

		} else {
			if (eop < 0) {
				i = (-eop);
			}
		}
	}

}

int msx_describe (int row)
{
	int i, pcount;
	int hd[HEADERSIZE];
	int en, tp, sp, lp;
	char bfname[NAMESIZE + 1], bfnameASCII[NAMESIZE + 1];
	int b;

	int s;
	int rd_err;


	en = ft[THISLOADER].en;
	tp = ft[THISLOADER].tp;
	sp = ft[THISLOADER].sp;
	lp = ft[THISLOADER].lp;

	/* Note: addblockdef() is the glue between ft[] and blk[], so we can now read from blk[] */
	s = blk[row] -> p2;

	/* Read header (it's safe to read it here for it was already decoded during the search stage) */
	for (i = 0, pcount = 0; i < HEADERSIZE; i++) {
		b = msx_read_byte(s + pcount);
		hd[i]= b & 0xff;
		pcount += b >> 8;
	}

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

	blk[row]->cs = 0;
	blk[row]->cx = 0;
	blk[row]->ce = 0;

	sprintf(lin,"\n - File ID : $%02X", hd[0]);
	strcat(info, lin);

	/* Compute pilot & trailer lengths */

	/* pilot is in pulses... */
	blk[row]->trail_len = blk[row]->p4 - blk[row]->p3 - ((b >> 8) - 1);

	/* ... trailer in pulses */
	blk[row]->trail_len = 0;

	rd_err = 0;

	return rd_err;
}
