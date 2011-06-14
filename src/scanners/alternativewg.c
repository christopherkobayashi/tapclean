/*
 * alternativewg.c
 *
 * Part of project "tapclean". 
 *
 * A Commodore 64 tape remastering and data extraction utility.
 *
 * (C) 2006 Fabrizio Gennari.
 * based on freeload.c. (C) 2001-2006 Stewart Wilson, Subchrist Software.
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
#include <stdio.h>

#define HDSZ 6


void alternativewg_search(void)
{
	int i, sof, sod, eod, eof;
	int z, h, hd[HDSZ];
	int en, tp, sp, lp, sv;
	unsigned int s, x;

	en = ft[ALTERWG].en;
	tp = ft[ALTERWG].tp;
	sp = ft[ALTERWG].sp;
	lp = ft[ALTERWG].lp;
	sv = ft[ALTERWG].sv;

	if (!quiet)
		msgout("  Alternative World Games");
    
	for (i = 20; i < tap.len - 8; i++) {
		if ((z = find_pilot(i, ALTERWG)) > 0) {
			sof = i;
			sod = i = z + 1;
			/* decode the header so we can validate the addresses... */

			for (h = 0; h < HDSZ; h++)
			{
				hd[h] = readttbyte(sod + (h * 8), lp, sp, tp, en);
				if (hd[h] == -1) /* fail in case of byte read error */
					break;
			}
			if (h != HDSZ || hd[1] != 0x20)
				continue;

			s = hd[2] + (hd[3] << 8);	/* get start address */
			x = hd[4] + (hd[5] << 8);	/* get length */
			eod = sod + ((x + HDSZ) * 8) - 1;
			eof = eod;
			addblockdef(ALTERWG, sof, sod, eod, eof, 0);
			i = eof;	/* optimize search */
		}
		else
		{
			if(z<0)    /* find_pilot failed (too few/many), set i to failure point. */
				i=(-z);
		}
	}
}

int alternativewg_describe(int row)
{
	int i, s, b, hd[HDSZ];
	int en, tp, sp, lp;

	en = ft[ALTERWG].en;
	tp = ft[ALTERWG].tp;
	sp = ft[ALTERWG].sp;
	lp = ft[ALTERWG].lp;
      
	/* decode header... */

	s = blk[row]->p2;
	for (i = 0; i < HDSZ; i++)
		hd[i] = readttbyte(s + (i * 8), lp, sp, tp, en);

	blk[row]->fn = (char*)malloc(hd[0]<10 ? 2 : hd[0]<100 ? 3: 4);
	sprintf(blk[row]->fn, "%u", hd[0]);

	blk[row]->cs = hd[2] + (hd[3] << 8);
	blk[row]->cx = hd[4] + (hd[5] << 8);
	blk[row]->ce = (blk[row]->cs + blk[row]->cx) - 1;

	/* get pilot & trailer lengths */

	blk[row]->pilot_len = (blk[row]->p2 - blk[row]->p1 - 8) >> 3;
	blk[row]->trail_len = 0;

	/* extract data ... */

	s = blk[row]->p2 + (HDSZ * 8);

	if (blk[row]->dd != NULL)
		free(blk[row]->dd);

	blk[row]->dd = (unsigned char*)malloc(blk[row]->cx);

	for (i = 0; i < blk[row]->cx; i++) {
		b = readttbyte(s + (i * 8), lp, sp, tp, en);
		if (b == -1)
			blk[row]->rd_err++;
		blk[row]->dd[i] = b;
	}

	return 0;
}

