/*
 * tap2audio.h
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
 *
 */   
   

#include <stdio.h>

#define BUFSZ	1048576	/* buffer size = 1 MB */
#define FREQ	44100
#define AUHDSZ	24	/* Size of .AU file header. */
#define WAVHDSZ	44	/* Size of .WAV file header. */

int au_write(unsigned char *, int, const char *, char);
int wav_write(unsigned char *, int, const char *, char);
int drawwavesquare(int, int, char, FILE *, unsigned long *, char *);
int drawwavesine(int, int, char, FILE *, unsigned long *, char *);
int s_out(unsigned char, int, FILE *, unsigned long *, char *);

