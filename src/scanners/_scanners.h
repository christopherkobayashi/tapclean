/*
 * scanners.h   
 *
 * Part of project "Final TAP". 
 *
 * A Commodore 64 tape remastering and data extraction utility.
 *
 * (C) 2001-2006 Stewart Wilson, Subchrist Software.
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
 */
    
void pause_search(void);
void pause_describe(int);
  
int cbm_readbit(int);
int cbm_readbyte(int);
void cbm_search(void);
int cbm_describe(int);
    
void turbotape_search(void);
int turbotape_describe(int);
    
void freeload_search(void);
int freeload_describe(int);

void odeload_search(void);
int odeload_describe(int);
    
void usgold_search(void);
int usgold_describe(int);
    
void nova_search(void);
int nova_describe(int);
    
void nova_spc_search(void);
int nova_spc_describe(int);
    
void aces_search(void);
int aces_describe(int);
    
void wild_search(void);
int wild_describe(int);
    
void chr_search(void);
int chr_describe(int);
    
void ocean_search(void);
int ocean_describe(int);
    
void raster_search(void);
int raster_describe(int);
    
int visiload_readbyte(int, int, int);
void visiload_search(void);
int visiload_describe(int);
    
void cyberload_f1_search(void);
int cyberload_f1_describe(int);
    
void cyberload_f2_search(void);
int cyberload_f2_describe(int);
    
void cyberload_f3_search(void);
int cyberload_f3_describe(int);
    
void cyberload_f4_search(void);
int cyberload_f4_describe(int);
    
void bleep_search(void);
int bleep_describe(int);
    
void bleep_spc_search(void);
int bleep_spc_describe(int);
    
void hitload_search(void);
int hitload_describe(int);
    
void micro_search(void);
int micro_describe(int);
    
void burner_search(void);
int burner_describe(int);
    
void rackit_search(void);
int rackit_describe(int);
    
int superpav_readbyte(int, int, int, int, int);
void superpav_search(void);
int superpav_describe(int);
    
void anirog_search(void);
int anirog_describe(int);
    
int supertape_readbyte(int, int);
void supertape_search(void);
int supertape_describe(int);
    
int pav_readbyte(int);
void pav_search(void);
int pav_describe(int);
 
void ik_search(void);
int ik_describe(int);
    
void firebird_search(void);
int firebird_describe(int);
    
void turrican_search(void);
int turrican_describe(int);
    
void seuck1_search(void);
int seuck1_describe(int);
    
void jetload_search(void);
int jetload_describe(int);
    
void flashload_search(void);
int flashload_describe(int);
    
void virgin_search(void);
int virgin_describe(int);
    
void hitec_search(void);
int hitec_describe(int);
    
void tdi_search(void);
int tdi_describe(int);
    
void oceannew1t1_search(void);
int oceannew1t1_describe(int);
    
void oceannew1t2_search(void);
int oceannew1t2_describe(int);
    
void atlantis_search(void);
int atlantis_describe(int row);
 
void snakeload51_search(void);
int snakeload51_describe(int);
    
void snakeload50t1_search(void);
int snakeload50t1_describe(int);
    
void snakeload50t2_search(void);
int snakeload50t2_describe(int);
    
void palacef1_search(void);
int palacef1_describe(int);
    
void palacef2_search(void);
int palacef2_describe(int);
    
void oceannew2_search(void);
int oceannew2_describe(int);
    
void enigma_search(void);
int enigma_describe(int);

void audiogenic_search(void);
int audiogenic_describe(int);

void cult_search(void);
int cult_describe(int);
   
void aliensyndrome_search(void);
int aliensyndrome_describe(int);
   
