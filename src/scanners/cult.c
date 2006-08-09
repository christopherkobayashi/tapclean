#include "../mydefs.h"
#include "../main.h"

#define SYNC 0xAA

void cult_search(void)
{
	int i, sof, sod, eod, eof;
	int z, ib;
	int en, tp, sp, lp, sv;
	unsigned int s, e, x;

	en = ft[CULT].en;
	tp = ft[CULT].tp;
	sp = ft[CULT].sp;
	lp = ft[CULT].lp;
	sv = ft[CULT].sv;

	if (!quiet)
		msgout(" Cult");


	ib = find_decode_block(CBM_DATA, 1);
	if (ib == -1)
		return;		/* failed to locate cbm data. */

	s = blk[ib]->dd[10] + (blk[ib]->dd[14] << 8); /* 0x0801 */

	ib = find_decode_block(CBM_HEAD, 1);
	if (ib == -1)
		return;		/* failed to locate cbm header. */

	e = blk[ib]->dd[27] + (blk[ib]->dd[31] << 8);

	/* sprintf(lin,"Cult end address found: $%x\n", e); */
	/* msgout(lin); */

	for (i = 20; i < tap.len - 8; i++) {
		if ((z = find_pilot(i, CULT)) > 0) {
			sof = i;
			i = z;

			if (readttbit(i, lp, sp, tp) == sv) {	/* got sync bit? */
				i++;
				if (readttbyte(i, lp, sp, tp, en) == SYNC) {	/* got sync byte?. */
					sod = i + 8;

					/* byte 0 + 1: start of code */
					/* byte 2 + 3: line number in basic */
					/* byte 4: Basic token for SYS */
					/* byte 5 - 11: ascii for 2061 + 3 x zero */

					if (e > s) {
						x = e - s;		/* compute length */
						eod = sod + (x * 8);
						eof = eod - 1;

						if (readttbit(eof + 1, lp, sp, tp) == 0)
							while (tap.tmem[eof + 1] > sp - tol &&
									tap.tmem[eof + 1] < sp + tol &&
									eof < tap.len - 1)
								eof++;

/*						while (readttbit(eof, lp, sp, tp) == 0) */
/*							eof++; */

						addblockdef(CULT, sof, sod, eod, eof, 0);
						i = eof;		/* optimize search */
					}
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
	int i, s, b, ib;
	int en, tp, sp, lp;

	en = ft[CULT].en;
	tp = ft[CULT].tp;
	sp = ft[CULT].sp;
	lp = ft[CULT].lp;

	ib = find_decode_block(CBM_DATA, 1);
	blk[row]->cs = blk[ib]->dd[10] + (blk[ib]->dd[14] << 8); /* 0x0801 */
	ib = find_decode_block(CBM_HEAD, 1);
	blk[row]->ce = blk[ib]->dd[27] + (blk[ib]->dd[31] << 8) - 1;
	blk[row]->cx = (blk[row]->ce - blk[row]->cs) + 1;

	/* get pilot & trailer lengths */

	blk[row]->pilot_len = (blk[row]->p2 - blk[row]->p1 - 8);
	blk[row]->trail_len = blk[row]->p4 - blk[row]->p3;

	/* extract data */

	s = blk[row]->p2;

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

