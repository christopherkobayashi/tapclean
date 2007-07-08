#ifndef _mydefs_h
#define _mydefs_h

/*
 * mydefs.h
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
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <malloc.h>
#include <time.h>

#include "database.h"
#include "scanners/_scanners.h"

/* OS dependent APIs and slash */

#ifdef WIN32
#define OSAPI_DELETE_FILE "del"
#define OSAPI_RENAME_FILE "ren"
#define SLASH   '\\'
#else
#define OSAPI_DELETE_FILE "rm"
#define OSAPI_RENAME_FILE "mv"
#define SLASH   '/'
#endif
#define OSAPI_CREATE_FOLDER "mkdir"

#define VERSION_STR "TAPClean v0.21 Console - (C) 2006-2007 XXX"
#define BUILDER     "bgk"

#define TRUE	1
#define FALSE	0

#define MAXPATH	512
#define NUM_READ_ERRORS 100
#define NUM_CRC 1000

#define CLEANED_PREFIX	"clean."

#define TAP_HEADER_SIZE 20
#define TAP_ID_STRING	"C64-TAPE-RAW"

#define DC2N_HEADER_SIZE	20
#define DC2N_ID_STRING		"DC2N-TAP-RAW"

#define FAIL	0xFFFFFFFF
#define DEFTOL	11	/* default bit reading tolerance. (1 = zero tolerance) */
#define MAXTOL  16      /* max bit reading tolerance (as above, its bias is 1) */

#define LAME	0x0F	/* cutoff value for 'noise' pulses when rebuilding pauses.*/

/* CPU cycles per second for C64, C16 and VIC20 PAL and NTSC */

#define C64_PAL_CPS	985248
#define C64_NTSC_CPS	1022727
#define C16_PAL_CPS	886724
#define C16_NTSC_CPS	894886
#define VIC20_PAL_CPS	1108405
#define VIC20_NTSC_CPS	1022727

#define NA	-1	/* indicator: Not Applicable. */
#define VV	-1	/* indicator: A variable value is used. */
#define XX	-1	/* indicator: Dont care. */

#define LSbF	0	/* indicator: Least Significant bit First. */
#define MSbF	1	/* indicator: Most Significant bit First. */
#define CSYES	1	/* indicator: A checksum is used. */
#define CSNO	0	/* indicator: A checksum is not used. */


/* each of these constants indexes an entry in the "ft[]" fmt_t array... */

enum {	GAP=1, PAUSE, CBM_HEAD, CBM_DATA, TT_HEAD, TT_DATA, FREE, ODELOAD,
	CULT, USGOLD, ACES, WILD,WILD_STOP,NOVA, NOVA_SPC, OCEAN_F1, OCEAN_F2,
	OCEAN_F3, CHR_T1, CHR_T2, CHR_T3, RASTER, CYBER_F1, CYBER_F2, CYBER_F3,
	CYBER_F4_1, CYBER_F4_2, CYBER_F4_3, BLEEP, BLEEP_TRIG, BLEEP_SPC,
	HITLOAD, MICROLOAD, BURNER, RACKIT, SPAV1_HD, SPAV1, SPAV2_HD, SPAV2,
	VIRGIN, HITEC, ANIROG, VISI_T1, VISI_T2, VISI_T3, VISI_T4,
	SUPERTAPE_HEAD, SUPERTAPE_DATA, PAV, IK, FBIRD1, FBIRD2, TURR_HEAD,
	TURR_DATA, SEUCK_L2, SEUCK_HEAD, SEUCK_DATA, SEUCK_TRIG, SEUCK_GAME,
	JET, FLASH, TDI_F1, OCNEW1T1, OCNEW1T2, OCNEW2, ATLAN, SNAKE51,
	SNAKE50T1, SNAKE50T2, PAL_F1, PAL_F2, ENIGMA, AUDIOGENIC, ALIENSY,
	ACCOLADE, ALTERWG, RAINBOWARTSF1, RAINBOWARTSF2, TRILOGIC, BURNERVAR,
	OCNEW4,TDI_F2
};

/*
 * These constants are the loader IDs, used for quick scanning via CRC lookup...
 * see file "loader_id.c"
 * see also string array 'knam' in the main file.
 */

enum {	LID_FREE=1, LID_ODE, LID_BLEEP, LID_CHR, LID_BURN, LID_WILD, LID_USG,
	LID_MIC, LID_ACE, LID_T250, LID_RACK, LID_OCEAN, LID_RAST, LID_SPAV,
	LID_HIT, LID_ANI, LID_VIS1, LID_VIS2, LID_VIS3, LID_VIS4, LID_FIRE,
	LID_NOVA, LID_IK, LID_PAV, LID_CYBER, LID_VIRG, LID_HTEC, LID_FLASH,
	LID_SUPER, LID_OCNEW1T1, LID_OCNEW1T2, LID_ATLAN, LID_SNAKE,
	LID_OCNEW2, LID_AUDIOGENIC, LID_CULT, LID_ACCOLADE, LID_RAINBOWARTS,
	LID_BURNERVAR,  LID_OCNEW4
};


/* struct 'tap_t' contains general info about the loaded tap file... */

struct tap_t
{
	char path[MAXPATH];	/* file path + name. */
	char name[MAXPATH];	/* file name. */
	unsigned char *tmem;	/* storage for the loaded tap. */
	int len;		/* length of the loaded tap. */
	int pst[256];		/* pulse stats table. */
	int fst[256];		/* file stats table. */
	int fsigcheck;		/* header check results... (1=ok, 0=failed) */
	int fvercheck;
	int fsizcheck;
	int detected;		/* number of bytes accounted for. */
	int detected_percent;	/* ..and as a percentage.*/
	int purity;		/* number of pulse types in the tap. */
	int total_gaps;		/* number of gaps. */
	int total_data_files;	/* number of files found. (not inc pauses and gaps) */
	int total_checksums;	/* number of files found with checksums. */
	int total_checksums_good;	/* number of checksums verified. */
	int optimized_files;	/* number of fully optimized files. */
	int total_read_errors;	/* number of read errors. */
	int fdate;		/* age of file. (date and time stamp). */
	float taptime;		/* playing time (seconds). */
	int version;		/* TAP version (currently 0 or 1). */
	int bootable;		/* holds the number of bootable ROM file sequences */
	int changed;		/* flags that the tap has been altered (+ needs rescan) */
	unsigned long crc;	/* overall (data extraction) crc32 for this tap */
	unsigned long cbmcrc;	/* crc32 of the 1st CBM program found */
	int cbmid;		/* loader id, see enums in mydefs.h (-1 = N/A) */
	char cbmname[20];	/* filename for first CBM file (if exists). */
	int tst_hd;
	int tst_rc;
	int tst_op;		/* quality test results. 0=Pass, 1= Fail */
	int tst_cs;
	int tst_rd;
};
extern struct tap_t tap;


/* a reduced version of the above used as a database by batch scan... */

struct tap_tr
{
	char path[MAXPATH];	/* full file path + name. */
	char name[MAXPATH];	/* file name. */
	int len;		/* length of the loaded tap. */
	int detected_percent;	/* ..and as a percentage. */
	int purity;		/* number of pulse types in the tap. */
	int total_data_files;	/* not including pauses or gaps. */
	int total_gaps;		/* number of gaps. */
	int fdate;		/* age of file. (date and time stamp). */
	int version;		/* TAP version (currently 0 or 1). */
	unsigned long crc;	/* overall (data extraction) crc32 for this tap */
	unsigned long cbmcrc;	/* crc32 of the 1st CBM program found  */
	int cbmid;		/* loader id, see enums in mydefs.h (-1 = N/A) */
	char cbmname[20];	/* filename for first CBM file (if exists). */
	int tst_hd;
	int tst_rc;
	int tst_op;		/* quality test results. 0=Pass, 1= Fail */
	int tst_cs;
	int tst_rd;
};


/* struct 'fmt_t' contains info about a particular tape format... */

struct fmt_t
{
	char name[32];		/* format name */
	int en;			/* byte endianess, 0=LSbF, 1=MSbF */
	int tp;			/* threshold pulsewidth (if applicable) */
	int sp;			/* ideal short pulsewidth */
	int mp;			/* ideal medium pulsewidth (if applicable) */
	int lp;			/* ideal long pulsewidth */
	int pv;			/* pilot value */
	int sv;			/* sync value */
	int pmin;		/* minimum pilots that should be present. */
	int pmax;		/* maximum pilots that should be present. */
	int has_cs;		/* flag, provides checksums, 1=yes, 0=no. */
};
extern struct fmt_t ft[100];


extern unsigned char cbm_header[192];		/* some formats must have their loader... */
extern unsigned char cbm_program[65536];	/* interrogated. */
extern int cbm_decoded;				/* 1= yes, 0= no */

extern int aborted;		/* general 'operation aborted' flag */
extern int tol;			/* turbotape bit reading tolerance. */
extern char lin[64000];		/* general purpose string building buffer */
extern char info[1048576];	/* string building buffer, gathers output from scanners */

extern const char knam[100][32];

extern int quiet;

extern int visi_type;		/* default visiload type.  */

extern int cyber_f2_eor1;
extern int cyber_f2_eor2;

extern int batchmode;

/* These are defined in main.c (move to main.h and include main.h here) */

extern const char temptcbatchreportname[];
extern const char tcbatchreportname[];
extern char exedir[MAXPATH];	/* assigned in main.c, includes trailing slash. */


/* program options... */

extern char debug;
extern char noid;
extern char noc64eof;
extern char docyberfault;
extern char boostclean;
extern char prgunite;
extern char extvisipatch;
extern char incsubdirs;
extern char sortbycrc;

extern char exportcyberloaders;

extern int dbase_is_full;

#endif

