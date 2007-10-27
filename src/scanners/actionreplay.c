/*---------------------------------------------------------------------------
  actionreplay.c
  derived from turbotape_easytape.c, iAN CooG/HF

  Part of project "Final TAP".

  A Commodore 64 tape remastering and data extraction utility.

  (C) 2001-2006 Stewart Wilson, Subchrist Software.



   This program is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free Software
   Foundation; either version 2 of the License, or (at your option) any later
   version.

   This program is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
   PARTICULAR PURPOSE. See the GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with
   this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
   St, Fifth Floor, Boston, MA 02110-1301 USA


  Notes:-


AR mk3 loader Found in some "algasoft" tapes
also found info about AR mk4 and later versions here:
http://www.geocities.com/SiliconValley/Platform/8224/c64tape/actionreplay.txt
Action Replay cartridge loader
==============================
  info by Fabrizio Gennari <fabrizio.ge@tiscalinet.it>

Used by the Action Replay cartridge to save freezed games.

Encoding:
---------

In the pilot tone, sync sequence and first bytes of header, pulses are like this:
 Threshold: $1D0 (464) clock cycles
 Bit 0: TAP byte $23
 Bit 1: TAP byte $53

Data Endianess: LEAST Significant bit First

 Lead-In :  a stream of 1's, typically about 2048 of them, followed by a single 0 bit
 Sync sequence:  $52, $42

Structure:
----------
 00-01 Complement of length of data (High, Low byte)
 02-03 Start Address of data        (High, Low byte)
 04    Checksum (XOR of all data bytes)
 05    discarded (typically $00)
 06    Low byte of threshold for data bytes
 07    discarded (typically $00)
 08    Data bytes

To calculate the number of data bytes, do 65536-complement of length. To
calculate threshold for data bytes, do $100+low byte. There are two
possibilities for the threshold: $1D0 (in that case, following pulses are
identical to previous ones: this is the case of Turbo backup) and $111 (in that
case, pulses are TAP bytes $13 for 0 and $2B for 1: that is the case of
Superturbo backup)
---------------------------------------------------------------------------*/

#include "../mydefs.h"
#include "../main.h"

#define HDSZ 8
#define MAXCBMBACKTRACE 0x2A00  /* max amount of pulses between turbo file and the
                                   'FIRST' instance of its CBM data block.
                                   The typical value is less than this one */
/*---------------------------------------------------------------------------
*/
void ar_search(void)
{
    int i,cnt2,sof,sod,eod,eof,z,ldrtype,nseq;
    int t,bufszh,bufszd,s,e,x,j;
    int cbmheader=0,cbmdata=0,easy2retry=0;
    unsigned int xinfo=0;
    unsigned char pat[32],hd[8];
    unsigned char *bufh=NULL,*bufd=NULL;
	int lp;
    int sp;
    int tp;
    int en;


    if(!quiet)
        msgout("  ActionReplay");
    /* scan the tape finding the 1st cbmheader containing a easytape */
    cbmheader=1;
    cbmdata=1;

    for(i=20; i<tap.len-8; i++)
    {
        t= find_decode_block(CBM_HEAD,cbmheader);

        if(t!=-1)
        {
            if(blk[t]->p1 < i-MAXCBMBACKTRACE)
            {
                i--;
                cbmheader++;
                continue;
            }
        }

        if(t!=-1)
        {
            bufszh= blk[t]->cx;
            bufh= malloc(bufszh*sizeof(int));
            for(j=0; j<bufszh; j++)
                bufh[j]= blk[t]->dd[j];
        }
        else
        {
            if (bufd) free(bufd);
            if (bufh) free(bufh);
            return;  /* there is no further cbmheader, let's get outta here*/
        }
        i=blk[t]->p4;
        while(t!=-1)
        {
            t= find_decode_block(CBM_DATA,cbmdata);
            if(t!=-1)
            {
                if(blk[t]->p1 < i-MAXCBMBACKTRACE)
                {
                    cbmdata++;
                    continue;
                }
                break;
            }
        }

        if(t!=-1)
        {
            bufszd= blk[t]->cx;
            bufd= malloc(bufszd*sizeof(int));
            for(j=0; j<bufszd; j++)
                bufd[j]= blk[t]->dd[j];
        }
        else
        {
            if (bufd) free(bufd);
            if (bufh) free(bufh);
            return;  /* there is no cbmdata, (weird) let's get outta here*/
        }
        j=(int)*(short int*)(bufh+1);
        ldrtype=0;
        switch(j)
        {
        case 0x326:
            ldrtype=ACTIONREPLAY;
            nseq=0x42;
            if(!quiet)
                msgout(" +AR3");
            break;
		case 0x2f0:
            ldrtype=ACTIONREPLAY;
            nseq=0x42;
            if(!quiet)
                msgout(" +AR4");
            break;
        default:
            ldrtype=0;
            break;
        }
        /* set new start point from end of this cbm data block anyway */
        i=blk[t]->p4;
        if(!ldrtype)
        {
            if (bufd){ free(bufd);bufd=NULL;}
            if (bufh){ free(bufh);bufh=NULL;}
            /* not a known header, retry from here */
            i--;
            cbmheader++;
        }
        else
        {
            /* found something, start from here */
            lp=ft[ldrtype].lp;
            sp=ft[ldrtype].sp;
            tp=ft[ldrtype].tp;
            en=ft[ldrtype].en;
            break;
        }
    }


    for(/*i=20*/; i<tap.len-8; i++)
    {
		if((z=find_pilot(i,ldrtype))>0)
        {
            sof=i;
            i=z;
            /* sync = $52+$4x */
            for(cnt2=0; cnt2<2; cnt2++)
            {
                pat[cnt2] = readttbyte(i+(cnt2*8), lp, sp, tp, en);
            }

            if( pat[0]==ft[ldrtype].sv && pat[1]==nseq )
            {
                i=i+(2*8);
                sod=i;
				for(cnt2=0; cnt2<8; cnt2++)
				{
					hd[cnt2]=readttbyte(i+(cnt2*8), lp, sp, tp, en);
				}
				s = hd[3]|(hd[2]<<8);
				e = s+(0xffff-(hd[1]|(hd[0]<<8)));
				xinfo=hd[6];
				/*
				hd[0],hd[1] hi/lo reciprocal of filesize
				            the loader INCrements $AE/AF and continues loading until they're 0000
				hd[2],hd[3] hi/lo startaddr
				hd[4]=expected xorchecksum
				hd[5]=ignored, always $00
				hd[6]=$dd06 new value (so far it's always $11)
				hd[7]=ignored, always $00
				*/

                i=i+(cnt2*8);

                if(e>s)
                {
                    x=e-s-1;
                    eod=sod+(x*8);
                    eof=eod+48; /* MUST CHECK: sometimes there are 48 bytes not cleaned at end of tape */
                    //eof=eof+7;
                   	eof+=16;
                    addblockdef(ldrtype, sof,sod,eod,eof, xinfo);
                    xinfo=0;
                    i=eof;   /* optimize search */
                    /*
                    now search next cbmheader
                    COPIED FROM TOP
                    */
                    if (bufd){ free(bufd);bufd=NULL;}
                    if (bufh){ free(bufh);bufh=NULL;}

                    cbmheader+=2;

                    for(/*i=20*/; i<tap.len-8; i++)
                    {
                        t= find_decode_block(CBM_HEAD,cbmheader);

                        if(t!=-1)
                        {
                            if(blk[t]->p1 < i-MAXCBMBACKTRACE)
                            {
                                i--;
                                cbmheader++;
                                continue;
                            }
                        }

                        if(t!=-1)
                        {
                            bufszh= blk[t]->cx;
                            bufh= malloc(bufszh*sizeof(int));
                            for(j=0; j<bufszh; j++)
                                bufh[j]= blk[t]->dd[j];
                        }
                        else
                        {
                            if (bufd) free(bufd);
                            if (bufh) free(bufh);
                            return;  /* there is no further cbmheader, let's get outta here*/
                        }
                        i=blk[t]->p4;
                        while(t!=-1)
                        {
                            t= find_decode_block(CBM_DATA,cbmdata);
                            if(t!=-1)
                            {
                                if(blk[t]->p1 < i-MAXCBMBACKTRACE)
                                {
                                    cbmdata++;
                                    continue;
                                }
                                break;
                            }
                        }

                        if(t!=-1)
                        {
                            bufszd= blk[t]->cx;
                            bufd= malloc(bufszd*sizeof(int));
                            for(j=0; j<bufszd; j++)
                                bufd[j]= blk[t]->dd[j];
                        }
                        else
                        {
                            if (bufd) free(bufd);
                            if (bufh) free(bufh);
                            return;  /* there is no cbmdata, (weird) let's get outta here*/
                        }
                        j=(int)*(short int*)(bufh+1);
                        ldrtype=0;
                        switch(j)
                        {
                        case 0x326:
                            ldrtype=ACTIONREPLAY;
                            nseq=0x42;
                            if(!quiet)
                                msgout(" +AR3");
                            break;
                		case 0x2f0:
                            ldrtype=ACTIONREPLAY;
                            nseq=0x42;
                            if(!quiet)
                                msgout(" +AR4");
                            break;
                        default:
                            ldrtype=0;
                            break;
                        }
                        /* set new start point from end of this cbm data block anyway */
                        i=blk[t]->p4;
                        if(!ldrtype)
                        {
                            if (bufd){ free(bufd);bufd=NULL;}
                            if (bufh){ free(bufh);bufh=NULL;}
                            /* not a known header, retry from here */
                            i--;
                            cbmheader++;
                        }
                        else
                        {
                            /* found something, start from here */
                            lp=ft[ldrtype].lp;
                            sp=ft[ldrtype].sp;
                            tp=ft[ldrtype].tp;
                            en=ft[ldrtype].en;
                            break;
                        }
                    }
                }
            }

        }
    }

    if (bufd){ free(bufd);bufd=NULL;}
    if (bufh){ free(bufh);bufh=NULL;}
}
/*---------------------------------------------------------------------------
*/
int ar_describe(int row)
{

    int i,s,off;
    int b,cb,hd[8],rd_err;
    int ldrtype=blk[row]->lt;
	int lp=ft[ldrtype].lp;
    int sp=ft[ldrtype].sp;
    int tp=ft[ldrtype].tp;
    int en=ft[ldrtype].en;

    /* decode header... */
    s= blk[row]->p2;

    off=HDSZ;
    for(i=0; i<off; i++)
        hd[i]= readttbyte(s+(i*8), lp, sp, tp, en);
    /* compute C64 start address, end address and size...  */

	blk[row]->cs = hd[3]+ (hd[2]<<8);
	blk[row]->ce = blk[row]->cs +(0xffff-(hd[1]+ (hd[0]<<8)));
	blk[row]->cs_act=hd[4];
	if(hd[6]==0x11)
	{
		ldrtype=ACTIONREPLAY_SUPER;
		lp=ft[ldrtype].lp;
		sp=ft[ldrtype].sp;
		tp=ft[ldrtype].tp;
		en=ft[ldrtype].en;
	}

    blk[row]->cx = (blk[row]->ce - blk[row]->cs) + 1;
    /* get pilot trailer lengths...  */
    blk[row]->pilot_len= (blk[row]->p2- blk[row]->p1 -8) >>3;
    blk[row]->trail_len= (blk[row]->p4- blk[row]->p3 -7) >>3;

    /* extract data... */
    rd_err=0;
    s= (blk[row]->p2)+(off*8);
    cb=0;
    if(blk[row]->dd!=NULL)
        free(blk[row]->dd);
    blk[row]->dd = (unsigned char*)malloc(blk[row]->cx);

    for(i=0; i<blk[row]->cx; i++)
    {
        b=readttbyte(s+(i*8), lp, sp, tp, en);
        if(b==-1)
            rd_err++;

        cb^=b;

        blk[row]->dd[i]= b;
    }

	blk[row]->cs_exp= cb &0xFF;

    blk[row]->rd_err= rd_err;
    return 0;

}

