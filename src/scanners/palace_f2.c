/*---------------------------------------------------------------------------
  palace_f2.c

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


  Note : Exactly the same as Palace F1 but block size is 1 byte smaller (262).

---------------------------------------------------------------------------*/

#include "../mydefs.h"
#include "../main.h"

/*---------------------------------------------------------------------------
*/
void palacef2_search(void)
{
   int i,j,k,sof,sod,eod,eof,tmp,blocks;
   int z;
   int fsync[4]= {0x4A,0x50,0x47,0x29};    /* file sync sequence. */
   int bsync[5]= {0x4A,0x50,0x47,0x10};    /* block sync sequence. followed by block #. */
   
   if(!quiet)
      msgout("  Palace Tape F2");
         
   
   for(i=20; i<tap.len-8; i++)
   {
      if((z=find_pilot(i,PAL_F2))>0)
      {
         sof=i;
         i=z;
         if(readttbyte(i, ft[PAL_F2].lp, ft[PAL_F2].sp, ft[PAL_F2].tp, ft[PAL_F2].en)==ft[PAL_F2].sv)
         {
            for(j=0,k=0; j<4; j++)  /* check file sync sequence... */
            {
               if(readttbyte(i+(j*8), ft[PAL_F2].lp, ft[PAL_F2].sp, ft[PAL_F2].tp, ft[PAL_F2].en)==fsync[j])
                  k++;
            }

            if(k==4)  /* file syncs are present, check block sync sequence... */
            {
               sod= i+32;   /* sod points at 1st byte of first blocks sync. */
               tmp= sod;
               blocks=0;

               do
               {
                  for(j=0,k=0; j<4; j++)  /* check block sync sequence... */
                  {
                     if(readttbyte(tmp+(j*8), ft[PAL_F2].lp, ft[PAL_F2].sp, ft[PAL_F2].tp, ft[PAL_F2].en)==bsync[j])
                        k++;
                  }
                  if(k==4)    /* block syncs are present... */
                  {
                     tmp+=(262*8);  /* each block is 262 bytes long. */
                     blocks++;
                  }
               }
               while(k==4);   /* while there was a block last time. */

               tmp-=8;   /* tmp should now point at last blocks checkbyte. */
               eod=tmp;
               eof=eod+7;

               addblockdef(PAL_F2, sof,sod,eod,eof, blocks);
               i=eof;  /* optimize search */
            }
         }
      }
      else
      {
         if(z<0)    /* find_pilot() failed (too few/many), set i to failure point. */
            i=(-z);
      }
   }
}
/*---------------------------------------------------------------------------
*/
int palacef2_describe(int row)
{
   int i,j,s,b,cb,goodchecks,ddi,total_blocks;

   total_blocks= blk[row]->xi;

   blk[row]->cx= 256*total_blocks;

   if(blk[row]->dd!=NULL)
      free(blk[row]->dd);
   blk[row]->dd= (unsigned char*)malloc(blk[row]->cx);

   sprintf(lin,"\n - Total Blocks: %d",total_blocks);
   strcat(info,lin);

   s= blk[row]->p2 +(5*8);   /* note : skips 1st block header. */

   /* test checkbyte for each block + extract data... */
   goodchecks=0;
   ddi=0;
   for(i=0; i<total_blocks; i++)
   {
      cb=0;
      for(j=0; j<256; j++)
      {
         b= readttbyte(s+(i*262*8)+(j*8), ft[PAL_F2].lp, ft[PAL_F2].sp, ft[PAL_F2].tp, ft[PAL_F2].en);
         blk[row]->dd[ddi++]=b;
         if(b==-1)
            blk[row]->rd_err++;
         cb^=b;
      }
      b= readttbyte(s+(i*262*8)+(j*8), ft[PAL_F2].lp, ft[PAL_F2].sp, ft[PAL_F2].tp, ft[PAL_F2].en);  /* read checkbyte. */
      if(cb==b)
         goodchecks++;
   }
   sprintf(lin,"\n<BR> - Good Checkbytes: %d of %d",goodchecks,total_blocks);
   strcat(info,lin);

   blk[row]->cs_exp= total_blocks;   /* fake the overall checkbyte as a */
   blk[row]->cs_act= goodchecks;     /* count of good checkbytes. */
   
   /* get pilot & trailer length.. */
   blk[row]->pilot_len= ((blk[row]->p2- blk[row]->p1)>>3)-4;
   blk[row]->trail_len=0;
   
   return 0;
}





 
