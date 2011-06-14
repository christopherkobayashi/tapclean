/*
 * aces.c (Ace of Aces)
 *
 * Part of project "Final TAP". 
 *
 * A Commodore 64 tape remastering and data extraction utility.
 *
 * (C) 2001-2006 Stewart Wilson, Subchrist Software.
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
   

#include "../mydefs.h"
#include "../main.h"

#include <string.h>
#include <stdlib.h>

#define HDSZ 20


void aces_search(void)
{
	int i, j, sof, sod, eod, eof;
	int z, psize, hd[HDSZ];

	if (!quiet)
		msgout("  Ace of Aces tape");
   
	for (i = 20; i < tap.len; i++) {
		if ((z = find_pilot(i, ACES)) > 0) {
			sof = i;
			i = z;
			if (readttbyte(i, ft[ACES].lp, ft[ACES].sp, ft[ACES].tp, ft[ACES].en) == ft[ACES].sv) {
				sod = i + 8;

				/* decode first few to read the block size */
				for (j = 0; j < HDSZ; j++)
					hd[j] = readttbyte(sod + (j * 8), ft[ACES].lp, ft[ACES].sp, ft[ACES].tp, ft[ACES].en);
				psize = (hd[2] + (hd[3] << 8)) - (hd[0] + (hd[1] << 8));

				if (psize > 0) {
					eod = sod + (psize * 8) + (20 * 8);	/* 20*8 adds in header bytes, +7 adds the check. */
					eof = eod + 7;
					addblockdef(ACES, sof, sod, eod, eof, 0);
					i = eof;	/* optimize search */
				}
			}
		} else {
			if (z < 0)	/* find_pilot failed (too few/many), set i to failure point. */
				i = (-z);
		}
	}
}

int aces_describe(int row)
{
	int i, s;
	int hd[HDSZ], b, rd_err, cb;
	char fn[256], str[2000] = "";

	/* decode header...   */
	s = blk[row]->p2;
	for (i = 0; i < HDSZ; i++)
		hd[i] = readttbyte(s + (i * 8), ft[ACES].lp, ft[ACES].sp, ft[ACES].tp, ft[ACES].en);
   
	blk[row]->cs = hd[0] + (hd[1] << 8);			/* record start address */
	blk[row]->ce = hd[2] + (hd[3] << 8) - 1;		/* record end address */
	blk[row]->cx = (blk[row]->ce + 1) - blk[row]->cs;	/* record length */

	/* extract file name... */
	for( i = 0; i < 16; i++)
		fn[i] = hd[4 + i];
	fn[i] = 0;
	trim_string(fn);
	pet2text(str, fn);
	if (blk[row]->fn != NULL)
		free(blk[row]->fn);
	blk[row]->fn = (char*)malloc(strlen(str) + 1);
	strcpy(blk[row]->fn, str);

	/* get pilot & trailer lengths */
	blk[row]->pilot_len = (blk[row]->p2 - blk[row]->p1 - 8) >> 3;
	blk[row]->trail_len = (blk[row]->p4 - blk[row]->p3 - 7) >> 3;

	/* extract data and test checksum...  */
	rd_err = 0;
	cb = 0;
	s = (blk[row]->p2) + (20 * 8);

	if (blk[row]->dd != NULL)
		free(blk[row]->dd);
	blk[row]->dd = (unsigned char*)malloc(blk[row]->cx);

	for (i = 0; i < blk[row]->cx; i++) {
		b = readttbyte(s + (i * 8), ft[ACES].lp, ft[ACES].sp, ft[ACES].tp, ft[ACES].en);
		cb ^= b;
		if (b == -1)
			rd_err++;
		blk[row]->dd[i] = b;
	}

	b = readttbyte(s + (i * 8), ft[ACES].lp, ft[ACES].sp, ft[ACES].tp, ft[ACES].en);

	blk[row]->cs_exp = cb & 0xFF;
	blk[row]->cs_act = b;
	blk[row]->rd_err = rd_err;

	return 0;
}
