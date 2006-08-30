/*
 * burner_var.c (by Luigi Di Fraia, Aug 2006 - armaeth@libero.it)
 *
 * Part of project "TAPClean". May be used in conjunction with "Final TAP".
 *
 * Based on burner.c, which are part of "Final TAP".
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

#include "../mydefs.h"
#include "../main.h"

#define THISLOADER	BURNERVAR

#define BITSINABYTE	8	/* a byte is made up of 8 bits here */

#define SYNCSEQSIZE	1	/* amount of sync bytes */
#define MAXTRAILER      16	/* max amount of trailer pulses read in */

#define HEADERSIZE	4	/* size of block header */

#define LOADOFFSETH	1	/* load location (MSB) offset inside header */
#define LOADOFFSETL	0	/* load location (LSB) offset inside header */
#define ENDOFFSETH	3	/* end  location (MSB) offset inside header */
#define ENDOFFSETL	2	/* end  location (LSB) offset inside header */

#define CBMXORDECRYPT	0x59	/* use this value to decrypt CBM header bytes */

#define V1PILOTOFFSET	0x87	/* offset to p.v. inside CBM header */
#define V1SYNCOFFSET	0x92	/* offset to s.v. inside CBM header */
#define V1ENDIANOFFSET	0x82	/* offset to ROR/ROL OPC inside CBM header */

#define V2PILOTOFFSET	0x89	/* offset to p.v. inside CBM header */
#define V2SYNCOFFSET	0x94	/* offset to s.v. inside CBM header */
#define V2ENDIANOFFSET	0x84	/* offset to ROR/ROL OPC inside CBM header */

#define OPC_ROL		0x26	/* 65xx ROL OPCode */
#define OPC_ROR		0x66	/* 65xx ROR OPCode */

void burnervar_search (void)
{
	int i, h;			/* counters */
	int sof, sod, eod, eof, eop;	/* file offsets */
	int hd[HEADERSIZE];		/* buffer to store block header info */

	int pv, sv;
	int en, tp, sp, lp;

	unsigned int s, e;		/* block locations referred to C64 memory */
	unsigned int x; 		/* block size */

	int ib, match;			/* condition variables */


	tp = ft[THISLOADER].tp;
	sp = ft[THISLOADER].sp;
	lp = ft[THISLOADER].lp;

	if(!quiet)
		msgout("  Burner (Mastertronic Variant)");

	/* First we retrieve the burner variables from the CBM header */
	ib = find_decode_block(CBM_HEAD, 1);
	if (ib == -1)
		return;    /* failed to locate cbm header. */

	match = 0;
	
	pv = blk[ib]->dd[V1PILOTOFFSET]  ^ CBMXORDECRYPT;
	sv = blk[ib]->dd[V1SYNCOFFSET]   ^ CBMXORDECRYPT;
	en = blk[ib]->dd[V1ENDIANOFFSET] ^ CBMXORDECRYPT;

	/* MSbF rol ($26) or.. LSbF ror($66)  */
	if (en == OPC_ROL || en == OPC_ROR)
		match = 1;
	
	/* Legacy Burner support */
/*	if (match == 0) {
		pv = blk[ib]->dd[0x88] ^ 0xCBMXORDECRYPT;
		sv = blk[ib]->dd[0x93] ^ 0xCBMXORDECRYPT;
		en = blk[ib]->dd[0x83] ^ 0xCBMXORDECRYPT;

		/* MSbF rol ($26) or.. LSbF ror($66)  */
/*		if(en == OPC_ROL || en == OPC_ROR)
			match = 3;
	}
*/
	if (match == 0) {
		pv = blk[ib]->dd[V2PILOTOFFSET] ^ CBMXORDECRYPT;
		sv = blk[ib]->dd[V2SYNCOFFSET] ^ CBMXORDECRYPT;
		en = blk[ib]->dd[V2ENDIANOFFSET] ^ CBMXORDECRYPT;

		/* MSbF rol ($26) or.. LSbF ror($66)  */
		if(en == OPC_ROL || en == OPC_ROR)
			match = 2;
	}

	/* Nothing to do, definitely not a Burner variant */
	if (match == 0)
		return;

	/* Convert en from the OPCode to any of the internally used values */
	en = (en == OPC_ROL) ? MSbF : LSbF;

	ft[THISLOADER].pv = pv; /* needed by find_pilot() */
	ft[THISLOADER].sv = sv; /* is this initialization really needed? */
	ft[THISLOADER].en = en; /* needed by find_pilot() */

	if(!quiet) {
		sprintf(lin, "  Burner variant found: pv=$%02X, sv=$%02X, en=%d", pv, sv, en);
		msgout(lin);
	}

	for (i = 20; i > 0 && i < tap.len - BITSINABYTE; i++) {
		eop = find_pilot(i, THISLOADER);

		if (eop > 0) {
			/* Valid pilot found, mark start of file */
			sof = i;
			i = eop;

			/* Check if there's a valid sync value for this loader */
			if (readttbyte(i, lp, sp, tp, en) != sv)
				continue;

			/* Valid sync found, mark start of data */
			sod = i + BITSINABYTE * SYNCSEQSIZE;

			/* Read header */
			for (h = 0; h < HEADERSIZE; h++) {
				hd[h] = readttbyte(sod + h * BITSINABYTE, lp, sp, tp, en);
				if (hd[h] == -1)
					break;
			}
			if (h != HEADERSIZE)
				continue;

			/* Extract load and end locations */
			s = hd[LOADOFFSETL] + (hd[LOADOFFSETH] << 8);
			e = hd[ENDOFFSETL]  + (hd[ENDOFFSETH]  << 8) - 1;

			/* Plausibility check */
			if (e < s)
				continue;

			/* Compute size */
			x = e - s + 1;

			/* Point to the first pulse of the exe ptr, MSB */
			/* Note: + 3 because there's a EOF marker and an exe ptr in v1 and v2 */
			if (match == 1 || match == 2)
				eod = sod + (HEADERSIZE + x + 3 - 1) * BITSINABYTE;
			/* Legacy Burner support */
/*			else
				eod = sod + (HEADERSIZE + x) * BITSINABYTE;*/

			/* Point to the last pulse of the exe ptr MSB/last data byte for Burner legacy */
			eof = eod + BITSINABYTE - 1;

			/* Trace 'eof' to end of trailer (any value, both bit 1 and bit 0 pulses)
			   Note: also check a different implementation that uses readttbit()) */
			h = 0;
			while (eof < tap.len - 1 && h++ < MAXTRAILER &&
					(tap.tmem[eof + 1] > sp - tol && /* no matter if overlapping occurrs here */
					tap.tmem[eof + 1] < sp + tol ||
					tap.tmem[eof + 1] > lp - tol && 
					tap.tmem[eof + 1] < lp + tol))
				eof++;

			/* Note: put discovered values in the extra-info field */
			if (addblockdef(THISLOADER, sof, sod, eod, eof, pv + (sv<<8)+ (en<<16)) >= 0)
				i = eof;	/* Search for further files starting from the end of this one */

		} else {
			if (eop < 0)
				i = (-eop);
		}
	}
}

int burnervar_describe (int row)
{
	int i, s;
	int hd[HEADERSIZE];

	int pv, sv;
	int en, tp, sp, lp;

	int b, rd_err;


	tp = ft[THISLOADER].tp;
	sp = ft[THISLOADER].sp;
	lp = ft[THISLOADER].lp;

	/* Retrieve the missing infos from the extra-info field of this block */
	pv = blk[row]->xi & 0xFF;
	sv = (blk[row]->xi & 0xFF00) >> 8;
	en = (blk[row]->xi & 0xFF0000) >> 16;

	sprintf(lin, "\n - Pilot : $%02X, Sync : $%02X, Endianess : ", pv, sv);
	strcat(info, lin);
	strcat(info, en == MSbF ? "MSbF" : "LSbF");

	/* Note: addblockdef() is the glue between ft[] and blk[], so we can now read from blk[] */
	s = blk[row] -> p2;

	/* Read header */
	for (i = 0; i < HEADERSIZE; i++)
		hd[i]= readttbyte(s + i * BITSINABYTE, lp, sp, tp, en);

	/* Extract load and end locations */
	blk[row]->cs = hd[LOADOFFSETL] + (hd[LOADOFFSETH] << 8);
	blk[row]->ce = hd[ENDOFFSETL]  + (hd[ENDOFFSETH]  << 8) - 1; /* we want the location of the last loaded byte */

	/* Compute size */
	blk[row]->cx = blk[row]->ce - blk[row]->cs + 1;

	/* Compute pilot & trailer lengths */

	/* pilot is in bytes... */
	blk[row]->pilot_len = (blk[row]->p2 - blk[row]->p1) / BITSINABYTE;

	/* ... trailer in pulses */
	blk[row]->trail_len = blk[row]->p4 - blk[row]->p3 - (BITSINABYTE - 1);

	/* if there IS pilot then disclude the sync byte */
	if(blk[row]->pilot_len > 0) 
		blk[row]->pilot_len -= SYNCSEQSIZE;

	/* Extract data and test checksum... */
	rd_err = 0;

	s = blk[row]->p2 + (HEADERSIZE * BITSINABYTE);

	if (blk[row]->dd!=NULL)
		free(blk[row]->dd);

   	blk[row]->dd = (unsigned char*)malloc(blk[row]->cx);

	for (i = 0; i < blk[row]->cx; i++) {
		b = readttbyte(s + (i * BITSINABYTE), lp, sp, tp, en);
		
		if(b != -1) {
			blk[row]->dd[i] = b;
		} else {
			blk[row]->dd[i] = 0x69;  /* error code */
			rd_err++;

			/* for experts only */
			sprintf(lin, "\n - Read Error on byte @$%X (prg data offset: $%04X)", s + (i * BITSINABYTE), i);
			strcat(info, lin);
		}
   	}

	/* EOF marker */
	b = readttbyte(s + (i * BITSINABYTE), lp, sp, tp, en);
	if (b != -1) {
		sprintf(lin, "\n - Marker : ");
		strcat(info, lin);
		strcat(info, b == 0 ? "EOF" : "not EOF");
	}
	else
		rd_err++;

	/* Suggestion: maybe also print the execution ptr */


	blk[row]->rd_err = rd_err;

	return(rd_err);
}
