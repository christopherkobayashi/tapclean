#include "../mydefs.h"
#include "../main.h"

#define HDSZ 4

void odeload_search(void)
{
	int i, sof, sod, eod, eof;
	int z, h, hd[HDSZ];
	int en, tp, sp, lp, sv;
	unsigned int s, e, x;

	en = ft[ODELOAD].en;	/* set endian according to table in main.c */
	tp = ft[ODELOAD].tp;	/* set threshold */
	sp = ft[ODELOAD].sp;	/* set short pulse */
	lp = ft[ODELOAD].lp;	/* set long pulse */
	sv = ft[ODELOAD].sv;	/* set sync value */

	if (!quiet)
		msgout("  ODEload");	/* output loader while searching */
				          
				         for(i=20; i<tap.len-8; i++)   // skip first 20 bytes of the TAP
						    {
							          if((z=find_pilot(i,ODELOAD))>0)  // call find_pilot according to the definition in main.c, if found go on...
									        {
											         sof=i;         // start of file = i
												          i=z;           // i = hmz.. check later :)
													           if(readttbyte(i,lp,sp,tp,en)==sv)
															            {
																	                sod=i+8;
																			            /* decode the header so we can validate the addresses... */
																			            for(h=0; h<HDSZ; h++)
																					                   hd[h] = readttbyte(sod+(h*8),lp,sp,tp,en);
																				                s=hd[0]+(hd[1]<<8);   /* get start address */
																						            e=hd[2]+(hd[3]<<8);   /* get end address */
																							                if(e>s)
																										            {
																												                   x=e-s;
																														                  eod=sod+((x+HDSZ)*8);
																																                 eof=eod+7;
																																		                addblockdef(ODELOAD, sof,sod,eod,eof, 0);
																																				               i=eof;  /* optimize search */
																																					                   }
																									         }
														         }
								        else
										      {
											               if(z<0)    /* find_pilot failed (too few/many), set i to failure point. */
													                   i=(-z);
												             }
									   }
}
/*---------------------------------------------------------------------------
 * */
int odeload_describe(int row)
{
	   int i,s,b,cb,hd[HDSZ];
	         
	      /* decode header... */
	      s= blk[row]->p2;
	         for(i=0; i<HDSZ; i++)
			       hd[i]= readttbyte(s+(i*8), ft[ODELOAD].lp, ft[ODELOAD].sp, ft[ODELOAD].tp, ft[ODELOAD].en);

		    blk[row]->cs= hd[0]+(hd[1]<<8);
		       blk[row]->ce= hd[2]+(hd[3]<<8)-1;
		          blk[row]->cx= (blk[row]->ce - blk[row]->cs)+1;  

			     /* get pilot & trailer lengths */
			     blk[row]->pilot_len= (blk[row]->p2- blk[row]->p1 -8)>>3;
			        blk[row]->trail_len=0;

				   /* extract data and test checksum... */
				   cb= 0;
				      s= blk[row]->p2+(HDSZ*8);

				         if(blk[row]->dd!=NULL)
						       free(blk[row]->dd);
					    blk[row]->dd= (unsigned char*)malloc(blk[row]->cx);

					       for(i=0; i<blk[row]->cx; i++)
						          {
								        b= readttbyte(s+(i*8), ft[ODELOAD].lp, ft[ODELOAD].sp, ft[ODELOAD].tp, ft[ODELOAD].en);
									      cb^= b;
									            if(b==-1)
											             blk[row]->rd_err++;
										          blk[row]->dd[i]= b;
											     }
					          b= readttbyte(s+(i*8), ft[ODELOAD].lp, ft[ODELOAD].sp, ft[ODELOAD].tp, ft[ODELOAD].en); /* read actual checkbyte. */

						     blk[row]->cs_exp= cb &0xFF;
						        blk[row]->cs_act= b;
							   return 0;
}
