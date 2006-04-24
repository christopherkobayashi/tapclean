#include "../mydefs.h"
#include "../main.h"

#define HDSZ 12

void cult_search(void)
{
	int i, sof, sod, eod, eof;
	int z, h, hd[HDSZ], ib;
	int en, tp, sp, lp, sv;
	unsigned int s, e, x;

	en = ft[CULT].en;
	tp = ft[CULT].tp;
	sp = ft[CULT].sp;
	lp = ft[CULT].lp;
	sv = ft[CULT].sv;

	if (!quiet)
		msgout(" Cult");


	ib = find_decode_block(CBM_HEAD, 1);
	if(ib == -1)
		return;		/* failed to locate cbm header. */

	s = 0x0801;
	e = blk[ib]->dd[27] + (blk[ib]->dd[31] << 8);

	/* sprintf(lin,"Cult end address found: $%x\n", e); */
	/* msgout(lin); */

	for (i = 20; i < tap.len - 8; i++) {
		if ((z = find_pilot(i, CULT)) > 0) {
			sof = i;
			i = z;

			if (readttbyte(i, lp, sp, tp, en) == sv) {
				sod = i + 8;

				/* decode the header, so we can validate the addresses */

				for (h = 0; h < HDSZ; h++)
					hd[h] = readttbyte(sod + (h * 8), lp, sp, tp, en);
				
				/* hd[0] + (hd[1] << 8);	start of code */
				/* hd[2] + (hd[3] << 8) = line number in basic */
				/* hd[4] = Basic token for SYS */
				/* hd[5] - hd[11] = ascii for 2061 + 3 x zero */

				if (e > s) {
					x = e - s;		/* compute length */
					eod = sod + (x * 8);
					eof = eod + 7;
					addblockdef(CULT, sof, sod, eod, eof, 0);
					i = eof;		/* optimize search */
				}
			}
		} else {
			if (z < 0)	/* find_pilot failed (too few/many), set i to failure point. */
				i = (-z);
		}
	}
}

int cult_describe(int row)
{
	int i, s, b, cb;
	int en, tp, sp, lp;

	en = ft[CULT].en;
	tp = ft[CULT].tp;
	sp = ft[CULT].sp;
	lp = ft[CULT].lp;
 
	blk[row]->cs = 0x0801;
	blk[row]->ce = 0xddaf;
	blk[row]->cx = (blk[row]->ce - blk[row]->cs) + 1;

	/* get pilot & trailer lengths */

	blk[row]->pilot_len = (blk[row]->p2 - blk[row]->p1 - 8) >> 3;
	blk[row]->trail_len = 0;

	/* extract data and test checksum... */

	cb = 0;
	s = blk[row]->p2;

	if (blk[row]->dd != NULL)
		free(blk[row]->dd);

	blk[row]->dd = (unsigned char*)malloc(blk[row]->cx);

	for (i = 0; i < blk[row]->cx; i++) {
		b = readttbyte(s + (i * 8), lp, sp, tp, en);
		cb ^= b;
		if (b == -1)
		blk[row]->rd_err++;
		blk[row]->dd[i] = b;
	}
	
	b = readttbyte(s + (i * 8), lp, sp, tp, en);	/* read actual checkbyte. */

	blk[row]->cs_exp = cb & 0xFF;
	blk[row]->cs_act = b;

	return 0;
}

