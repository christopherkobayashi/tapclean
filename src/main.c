/*
 * main.c  (Final TAP 2.7.x console version).
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


#include "main.h"
#include "mydefs.h"
#include "crc32.h"
#include "tap2audio.h"
#include "skewadapt.h"

#ifdef WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

/* program options... */

static char noaddpause		= FALSE;
static char sine		= FALSE;
static char doprg		= FALSE;

char debug			= FALSE;
char noid			= FALSE;
char noc64eof			= FALSE;
char docyberfault		= FALSE;
char boostclean			= FALSE;
char prgunite			= FALSE;
char extvisipatch		= FALSE;
char incsubdirs			= FALSE;
char sortbycrc			= FALSE;
char c16			= FALSE;
char c20			= FALSE;
char c64			= TRUE;
char pal			= TRUE;
char ntsc			= FALSE;
char exportcyberloaders		= FALSE;
char skewadapt			= FALSE;

static char preserveloadertable	= TRUE;

/*
 * Parameters -no/-do and descriptions
 *
 * See enum for this table in mydefs.h: these must be kept in sync.
 */
struct ldrswt_t ldrswt[] = {
	/* description (max 23 chars),   parameter,     exclude? */
	{"C64 ROM loader"		,"c64"		,FALSE},
	{"108DE0A5"			,"108DE0A5"	,FALSE},
	{"Accolade/EA"			,"accolade"	,FALSE},
	{"Ace of Aces"			,"aces"		,FALSE},
	{"ActionReplay"			,"ar"		,FALSE},
	{"Alien Syndrome"		,"aliensy"	,FALSE},
	{"Alternative World Games"	,"alterwg"	,FALSE},
	{"Anirog"			,"anirog"	,FALSE},
	{"Ash+Dave"			,"ashdave"	,FALSE},
	{"Atlantis"			,"atlantis"	,FALSE},
	{"Audiogenic"			,"audiogenic"	,FALSE},
	{"Biturbo"			,"biturbo"	,FALSE},
	{"Bleepload"			,"bleep"	,FALSE},
	{"Burner"			,"burner"	,FALSE},
	{"Burner Variant"		,"burnervar"	,FALSE},
	{"CHR"				,"chr"		,FALSE},
	{"Cult"				,"cult"		,FALSE},
	{"Cyberload"			,"cyber"	,FALSE},
	{"Enigma"			,"enigma"	,FALSE},
	{"Fast Evil"			,"fastevil"	,FALSE},
	{"FF Tape"			,"fftape"	,FALSE},
	{"Firebird"			,"fire"		,FALSE},
	{"Flashload"			,"flash"	,FALSE},
	{"Freeload"			,"free"		,FALSE},
	{"Freeload Slowload"		,"frslow"	,FALSE},
	{"Go For The Gold"		,"goforgold"	,FALSE},
	{"Hitload"			,"hit"		,FALSE},
	{"Hi-Tec"			,"hitec"	,FALSE},
	{"IK"				,"ik"		,FALSE},
	{"Jetload"			,"jet"		,FALSE},
	{"Microload"			,"micro"	,FALSE},
	{"Novaload"			,"nova"		,FALSE},
	{"Ocean"			,"ocean"	,FALSE},
	{"Ocean F1"			,"oceannew1t1"	,FALSE},
	{"Ocean F2"			,"oceannew1t2"	,FALSE},
	{"Ocean New 2"			,"oceannew2"	,FALSE},
	{"Ocean New 4"			,"oceannew4"	,FALSE},
	{"ODEload"			,"ode"		,FALSE},
	{"Palace F1"			,"palacef1"	,FALSE},
	{"Palace F2"			,"palacef2"	,FALSE},
	{"Pavloda"			,"pav"		,FALSE},
	{"Rack-It"			,"rackit"	,FALSE},
	{"Rainbow Arts F1"		,"rainbowf1"	,FALSE},
	{"Rainbow Arts F2"		,"rainbowf2"	,FALSE},
	{"Rasterload"			,"raster"	,FALSE},
	{"SEUCK"			,"seuck"	,FALSE},
	{"Snakeload 50"			,"snake50"	,FALSE},
	{"Snakeload 51"			,"snake51"	,FALSE},
	{"Super Pavloda"		,"spav"		,FALSE},
	{"Super Tape"			,"super"	,FALSE},
	{"TDI F1"			,"tdif1"	,FALSE},
	{"TDI F2"			,"tdif2"	,FALSE},
	{"TES Tape"			,"testape"	,FALSE},
	{"Trilogic"			,"trilogic"	,FALSE},
	{"Turbotape 250"		,"turbo"	,FALSE},
	{"Turrican"			,"turr"		,FALSE},
	{"U.S. Gold"			,"usgold"	,FALSE},
	{"Virgin"			,"virgin"	,FALSE},
	{"Visiload"			,"visi"		,FALSE},
	{"Wildload"			,"wild"		,FALSE}
	/* description (max 23 chars),   parameter,     exclude? */
};

static int read_errors[NUM_READ_ERRORS];	/* storage for 1st NUM_READ_ERRORS read error addresses */
static char note_errors;	/* set true only when decoding identified files, */
				/* it just tells 'add_read_error()' to ignore. */

struct tap_t tap;		/* container for the loaded tap (note: only ONE tap). */

int tol = DEFTOL;		/* bit reading tolerance. (1 = zero tolerance) */

char info[1048576];		/* buffer area for storing each blocks description text. */
				/* also used to store the database report, hence the size! (1MB). */
char lin[64000];		/* general purpose string building buffer. */
char tmp[64000];		/* general purpose string building buffer. */

int aborted = FALSE;		/* general 'operation aborted by user' flag. */

int visi_type = VISI_T2;	/* default visiload type, overidden when loader ID'd. */

int cyber_f2_eor1 = 0xAE;	/* default XOR codes for cyberload f2.. */
int cyber_f2_eor2 = 0xD2;

int batchmode = FALSE;		/* set to 1 when "batch analysis" is performed. */
				/* set to 0 when the user Opens an individual tap. */

unsigned char cbm_header[192];	/* these will allow some interrogation of the CBM parts */
unsigned char cbm_program[65536];	/* that some customized loaders may rely on. (ie. burner). */
int cbm_decoded = FALSE;	/* this ensures only *1st* cbm parts are decoded. */

int quiet = FALSE;		/* set 1 to stop the loader search routines from producing output, */
				/* ie. "Scanning: Freeload". */
				/* i set it (1) when (re)searching during optimization. */

//int dbase_is_full = 0;	/* used by 'addblockdef' to indicate database capacity reached. */

long cps = C64_PAL_CPS;		/* CPU Cycles pr second. Default is C64 PAL */


/*
 * Where a field is marked 'VV', loader/file interrogation is required to
 * discover the missing value.
 *
 * NOTE: some of the values (like number of pilot bytes) may not agree with
 * the loader docs, this is done to let partly damaged games be detected
 * and fixed.
 *
 * en = byte endianess, 0=LSbF, 1=MSbF
 * tp = threshold pulsewidth (if applicable)
 * sp = ideal short pulsewidth
 * mp = ideal medium pulsewidth (if applicable)
 * lp = ideal long pulsewidth
 * pv = pilot value
 * sv = sync value
 * pmin = minimum pilots that should be present.
 * pmax = maximum pilots that should be present.
 * has_cs = flag, provides checksums, 1=yes, 0=no.
 */

struct fmt_t ft_org[100];	/* a backup copy of the following... */
struct fmt_t ft[100] = {
	/* name (max 31 chars),  en,   tp,   sp,   mp,  lp,   pv,   sv,   pmin,pmax,  has_cs. */

	{""			,NA,   NA,   NA,   NA,  NA,   NA,   NA,   NA,  NA,    NA},
	{"UNRECOGNIZED"		,NA,   NA,   NA,   NA,  NA,   NA,   NA,   NA,  NA,    NA},
	{"PAUSE"		,NA,   NA,   NA,   NA,  NA,   NA,   NA,   NA,  NA,    NA},
	{"C64 ROM-TAPE HEADER"	,LSbF, NA,   0x30, 0x42,0x56, NA,   NA,   50,  NA,    CSYES},
	{"C64 ROM-TAPE DATA"	,LSbF, NA,   0x30, 0x42,0x56, NA,   NA,   50,  NA,    CSYES},
	{"TURBOTAPE-250 HEADER"	,MSbF, 0x20, 0x1A, NA,  0x28, 0x02, 0x09, 50,  NA,    CSNO},
	{"TURBOTAPE-250 DATA"	,MSbF, 0x20, 0x1A, NA,  0x28, 0x02, 0x09, 50,  NA,    CSYES},
	{"FREELOAD"		,MSbF, 0x2C, 0x24, NA,  0x42, 0x40, 0x5A, 45,  400,   CSYES},
	{"ODELOAD"		,MSbF, 0x36, 0x25, NA,  0x50, 0x20, 0xDB, 40,  NA,    CSYES},
	{"CULT"			,LSbF, 0x34, 0x27, NA,  0x3E, 0,    1,    1000,NA,    CSNO},
	{"US-GOLD TAPE"		,MSbF, 0x2C, 0x24, NA,  0x42, 0x20, 0xFF, 50,  NA,    CSYES},
	{"ACE OF ACES TAPE"	,MSbF, 0x2C, 0x22, NA,  0x47, 0x80, 0xFF, 50,  NA,    CSYES},
	{"WILDLOAD"		,LSbF, 0x3B, 0x30, NA,  0x47, 0xA0, 0x0A, 50,  NA,    CSYES},
	{"WILDLOAD STOP"	,LSbF, 0x3B, 0x30, NA,  0x47, 0xA0, 0x0A, 50,  NA,    CSNO},
	{"NOVALOAD"		,LSbF, 0x3D, 0x24, NA,  0x56, 0,    1,    1800,NA,    CSYES},
	{"NOVALOAD SPECIAL"	,LSbF, 0x3D, 0x24, NA,  0x56, 0,    1,    1800,NA,    CSYES},
	{"OCEAN/IMAGINE F1"	,LSbF, 0x3B, 0x24, NA,  0x56, 0,    1,    3000,13000, CSNO},
	{"OCEAN/IMAGINE F2"	,LSbF, 0x3B, 0x24, NA,  0x56, 0,    1,    3000,13000, CSNO},
	{"OCEAN/IMAGINE F3"	,LSbF, 0x3B, 0x24, NA,  0x56, 0,    1,    3000,13000, CSNO},
	{"CHR TAPE T1"		,MSbF, 0x20, 0x1A, NA,  0x28, 0x63, 0x64, 50,  NA,    CSYES},
	{"CHR TAPE T2"		,MSbF, 0x2D, 0x26, NA,  0x36, 0x63, 0x64, 50,  NA,    CSYES},
	{"CHR TAPE T3"		,MSbF, 0x3E, 0x36, NA,  0x47, 0x63, 0x64, 50,  NA,    CSYES},
	{"RASTERLOAD"		,MSbF, 0x3F, 0x26, NA,  0x58, 0x80, 0xFF, 20,  NA,    CSYES},
	{"CYBERLOAD F1"		,MSbF, VV,   VV,   VV,  VV,   VV,   VV,   50,  NA,    CSNO},
	{"CYBERLOAD F2"		,MSbF, VV,   VV,   VV,  VV,   VV,   VV,   20,  NA,    CSNO},
	{"CYBERLOAD F3"		,MSbF, VV,   VV,   VV,  VV,   VV,   VV,    7,   9,    CSYES},
	{"CYBERLOAD F4_1"	,MSbF, VV,   VV,   VV,  VV,   VV,   VV,    6,  11,    CSYES},
	{"CYBERLOAD F4_2"	,MSbF, VV,   VV,   VV,  VV,   VV,   VV,    6,  11,    CSYES},
	{"CYBERLOAD F4_3"	,MSbF, VV,   VV,   VV,  VV,   VV,   VV,    6,  11,    CSYES},
	{"BLEEPLOAD"		,MSbF, 0x45, 0x30, NA,  0x5A, VV,   VV,   50,  NA,    CSYES},
	{"BLEEPLOAD TRIGGER"	,MSbF, 0x45, 0x30, NA,  0x5A, VV,   VV,   50,  NA,    CSNO},
	{"BLEEPLOAD SPECIAL"	,MSbF, 0x45, 0x30, NA,  0x5A, VV,   VV,   50,  NA,    CSYES},
	{"HITLOAD"		,MSbF, 0x4E, 0x34, NA,  0x66, 0x40, 0x5A, 30,  NA,    CSYES},
	{"MICROLOAD"		,LSbF, 0x29, 0x1C, NA,  0x36, 0xA0, 0x0A, 50,  NA,    CSYES},
	{"BURNER TAPE"		,VV,   0x2F, 0x22, NA,  0x42, VV,   VV,   30,  NA,    CSNO},
	{"RACK-IT TAPE"		,MSbF, 0x2B, 0x1D, NA,  0x3D, VV,   VV,   50,  NA,    CSYES},
	{"SUPERPAV T1 HEADER"	,MSbF, NA,   0x2E, 0x45,0x5C, NA,   0x66, 50,  NA,    CSYES},
	{"SUPERPAV T1"		,MSbF, NA,   0x2E, 0x45,0x5C, NA,   0x66, 50,  NA,    CSYES},
	{"SUPERPAV T2 HEADER"	,MSbF, NA,   0x3E, 0x5D,0x7C, NA,   0x66, 50,  NA,    CSYES},
	{"SUPERPAV T2"		,MSbF, NA,   0x3E, 0x5D,0x7C, NA,   0x66, 50,  NA,    CSYES},
	{"VIRGIN TAPE"		,MSbF, 0x2B, 0x21, NA,  0x3B, 0xAA, 0xA0, 30,  NA,    CSYES},
	{"HI-TEC TAPE"		,MSbF, 0x2F, 0x25, NA,  0x45, 0xAA, 0xA0, 30,  NA,    CSYES},
	{"ANIROG TAPE"		,MSbF, 0x20, 0x1A, NA,  0x28, 0x02, 0x09, 50,  NA,    CSNO},
	{"VISILOAD T1"		,VV,   0x35, 0x25, NA,  0x48, 0x00, 0x16, 100, NA,    CSNO},
	{"VISILOAD T2"		,VV,   0x3B, 0x25, NA,  0x4B, 0x00, 0x16, 100, NA,    CSNO},
	{"VISILOAD T3"		,VV,   0x3E, 0x25, NA,  0x54, 0x00, 0x16, 100, NA,    CSNO},
	{"VISILOAD T4"		,VV,   0x47, 0x30, NA,  0x5D, 0x00, 0x16, 100, NA,    CSNO},
	{"SUPERTAPE HEADER"	,LSbF, NA,   0x21, 0x32,0x43, 0x16, 0x2A, 50,  NA,    CSYES},
	{"SUPERTAPE DATA"	,LSbF, NA,   0x21, 0x32,0x43, 0x16, 0xC5, 50,  NA,    CSYES},
	{"PAVLODA"		,MSbF, 0x28, 0x1F, NA,  0x3F, 0,    1,    50,  NA,    CSYES},
	{"IK TAPE"		,MSbF, 0x2C, 0x27, NA,  0x3F, 1,    0,    1000,NA,    CSYES},
	{"FIREBIRD T1"		,LSbF, 0x62, 0x44, NA,  0x7E, 0x02, 0x52, 30,  NA,    CSYES},
	{"FIREBIRD T2"		,LSbF, 0x52, 0x45, NA,  0x65, 0x02, 0x52, 30,  NA,    CSYES},
	{"TURRICAN TAPE HEADER"	,MSbF, NA,   0x1B, NA,  0x27, 0x02, 0x0C, 30,  NA,    CSNO},
	{"TURRICAN TAPE DATA"	,MSbF, NA,   0x1B, NA,  0x27, 0x02, 0x0C, 30,  NA,    CSYES},
	{"SEUCK LOADER 2"	,LSbF, 0x2D, 0x20, NA,  0x41, 0xE3, 0xD5, 5,   NA,    CSYES},
	{"SEUCK HEADER"		,LSbF, 0x2D, 0x20, NA,  0x41, 0xE3, 0xD5, 5,   NA,    CSNO},
	{"SEUCK SUB-BLOCK"	,LSbF, 0x2D, 0x20, NA,  0x41, 0xE3, 0xD5, 5,   NA,    CSYES},
	{"SEUCK TRIGGER"	,LSbF, 0x2D, 0x20, NA,  0x41, 0xE3, 0xD5, 5,   NA,    CSNO},
	{"SEUCK GAME"		,LSbF, 0x2D, 0x20, NA,  0x41, 0xE3, 0xAC, 50,  NA,    CSNO},
	{"JETLOAD"		,LSbF, 0x3B, 0x33, NA,  0x58, 0xD1, 0x2E, 1,   NA,    CSNO},
	{"FLASHLOAD"		,MSbF, NA,   0x1F, NA,  0x31, 1,    0,    50,  NA,    CSYES},
	{"TDI TAPE F1"		,LSbF, NA,   0x44, NA,  0x65, 0xA0, 0x0A, 50,  NA,    CSYES},
	{"OCEAN NEW TAPE F1 T1"	,MSbF, NA,   0x22, NA,  0x42, 0x40, 0x5A, 50,  200,   CSYES},
	{"OCEAN NEW TAPE F1 T2"	,MSbF, NA,   0x35, NA,  0x65, 0x40, 0x5A, 50,  200,   CSYES},
	{"OCEAN NEW TAPE F2"	,MSbF, 0x2C, 0x22, NA,  0x42, 0x40, 0x5A, 50,  200,   CSYES},
	{"ATLANTIS TAPE"	,LSbF, 0x2F, 0x1D, NA,  0x42, 0x02, 0x52, 50,  NA,    CSYES},
	{"SNAKELOAD 5.1"	,MSbF, NA,   0x28, NA,  0x48, 0,    1,    1800,NA,    CSYES},
	{"SNAKELOAD 5.0 T1"	,MSbF, NA,   0x3F, NA,  0x5F, 0,    1,    1800,NA,    CSYES},
	{"SNAKELOAD 5.0 T2"	,MSbF, NA,   0x60, NA,  0xA0, 0,    1,    1800,NA,    CSYES},
	{"PALACE TAPE F1"	,MSbF, NA,   0x30, NA,  0x57, 0x01, 0x4A, 50,  NA,    CSYES},
	{"PALACE TAPE F2"	,MSbF, NA,   0x30, NA,  0x57, 0x01, 0x4A, 50,  NA,    CSYES},
	{"ENIGMA TAPE"		,MSbF, 0x2C, 0x24, NA,  0x42, 0x40, 0x5A, 700, NA,    CSNO},
	{"AUDIOGENIC"		,MSbF, 0x28, 0x1A, NA,  0x36, 0xF0, 0xAA, 4,   NA,    CSYES},
	{"ALIEN SYNDROME"	,MSbF, 0x2C, 0x20, NA,  0x43, 0xE3, 0xED, 4,   NA,    CSYES},
	{"ACCOLADE"		,MSbF, 0x3D, 0x29, NA,  0x4A, 0x0F, 0xAA, 4,   NA,    CSYES},
	{"ALTERNATIVE WORLD G."	,MSbF, 0x4A, 0x33, NA,  0x65, 0x01, 0x00, 192, NA,    CSNO},
	{"RAINBOW ARTS F1"	,LSbF, 0x2A, 0x19, NA,  0x36, 0xA0, 0x0A, 800, NA,    CSYES},
	{"RAINBOW ARTS F2"	,LSbF, 0x2A, 0x19, NA,  0x36, 0xA0, 0x0A, 800, NA,    CSYES},
	{"TRILOGIC"		,MSbF, 0x28, 0x1C, NA,  0x35, 0x0F, 0x0E, 200, NA,    CSYES},
	{"BURNER VARIANT"	,VV,   0x30, 0x23, NA,  0x42, VV,   VV,   50,  NA,    CSNO},
	{"OCEAN NEW TAPE F4"	,MSbF, 0x2D, 0x24, NA,  0x44, 0x40, 0x5A, 50,  200,   CSYES},
	{"TDI TAPE F2"		,LSbF, NA,   0x44, NA,  0x65, 0xA0, 0x0A, 50,  NA,    CSYES},
	{"BITURBO"		,MSbF, 0x21, 0x1B, NA,  0x27, 0x02, 0x10, 400, NA,    CSYES},
	{"108DE0A5"		,LSbF, 0x1F, 0x1B, NA,  0x30, 0x02, 0x09, 200, NA,    CSYES},
	{"ACTIONREPLAY_HEADER"  ,LSbF, 0x3A, 0x23, NA,  0x53, 1,    0,    1500,NA,    CSNO},
	{"ACTIONREPLAY_TURBO"   ,LSbF, 0x3A, 0x23, NA,  0x53, NA,   NA,   NA,  NA,    CSYES},
	{"ACTIONREPLAY_SUPERTURBO"
				,LSbF, 0x22, 0x13, NA,  0x2B, NA,   NA,   NA,  NA,    CSYES},
	{"ASH AND DAVE"		,MSbF, 0x2D, 0x22, NA,  0x44, 0x80, 0x40, 200, NA,    CSNO},
	{"FREELOAD SLOWLOAD"	,MSbF, 0x77, 0x5A, NA,  0x85, 0x40, 0x5A, 45,  400,   CSYES},
	{"GO FOR THE GOLD"	,LSbF, 0x2F, 0x1D, NA,  0x42, 0x02, 0x11, 200, NA,    CSYES},
	{"FAST EVIL"		,MSbF, 0x1D, 0x17, NA,  0x21, 0x10, 0x20, 200, NA,    CSNO},
	{"FF TAPE"		,LSbF, 0x34, 0x28, NA,  0x3F, 0,    1,    1500,NA,    CSNO},
	{"TES TAPE"		,LSbF, 0x30, 0x1D, NA,  0x44, 0x02, 0x52, 200, NA,    CSYES},

	/* Closing record */
	{""			,666,  666,  666, 666,   666,  666,  666, 666, 666,   666}
	/* name (max 31 chars),  en,   tp,   sp,   mp,  lp,   pv,   sv,   pmin,pmax,  has_cs. */
};

/* 
 * The following strings are used to describe which loader signature has been 
 * found in CBM data file.
 *
 * See enum for this table in mydefs.h: these must be kept in sync.
 */

const char knam[][32] = {
	/*
	 * Only loaders with a LID_ entry in mydefs.h enums. Do not list 
	 * them all here!
	 */
	{"n/a"},
	{"Freeload (or clone)"},
	{"Odeload"},
	{"Bleepload"},
	{"CHR loader"},
	{"Burner"},
	{"Wildload"},
	{"US Gold loader"},
	{"Microload"},
	{"Ace of Aces loader"},
	{"Turbotape 250"},
	{"Rack-It loader"},
	{"Ocean/Imagine (ns)"},
	{"Rasterload"},
	{"Super Pavloda"},
	{"Hit-Load"},
	{"Anirog loader"},
	{"Visiload T1"},
	{"Visiload T2"},
	{"Visiload T3"},
	{"Visiload T4"},
	{"Firebird loader"},
	{"Novaload (ns)"},
	{"IK loader"},
	{"Pavloda"},
	{"Cyberload"},
	{"Virgin"},
	{"Hi-Tec"},
	{"Flashload"},
	{"Supertape"},
	{"Ocean New 1 T1"},
	{"Ocean New 1 T2"},
	{"Atlantis Loader"},
	{"Snakeload"},
	{"Ocean New 2"},
	{"Audiogenic"},
	{"Cult tape"},
	{"Accolade (or clone)"},
	{"Rainbow Arts (F1/F2)"},
	{"Burner (Mastertronic Variant)"},
	{"Ocean New 4"},
	{"108DE0A5"},
	{"Freeload Slowload"},
	{"Go For The Gold"},
	{"Fast Evil"},
	{"FF Tape"},
	{"TES Tape"}
	/*
	 * Only loaders with a LID_ entry in mydefs.h enums. Do not list 
	 * them all here!
	 */
};


/* note: all generated files are saved to the exedir */

static const char tcreportname[] =	"tcreport.txt";
static const char temptcreportname[] =	"tcreport.tmp";
static const char tcinfoname[] =	"tcinfo.txt";
static char cleanedtapname[MAXPATH];	/* assigned in main(). */

const char tcbatchreportname[] =	"tcbatch.txt";
const char temptcbatchreportname[] =	"tcbatch.tmp";
char exedir[MAXPATH];			/* global, assigned in main(), includes trailing slash. */


/*
 * Internal usage functions
 */

/*
 * Erase all stored data for the loaded 'tap', free buffers,
 * and reset database entries.
 */

static void unload_tap(void)
{
	int i;

	strcpy(tap.path, "");
	strcpy(tap.name, "");
	strcpy(tap.cbmname, "");
	if(tap.tmem != NULL) {
		free(tap.tmem);
		tap.tmem = NULL;
	}

	tap.len = 0;
	for(i = 0; i < 256; i++) {
		tap.pst[i] = 0;
		tap.fst[i] = 0;
	}
	
	tap.fsigcheck = 0;
	tap.fvercheck = 0;
	tap.fsizcheck = 0;
	tap.detected = 0;
	tap.detected_percent = 0;
	tap.purity = 0;
	tap.total_gaps = 0;
	tap.total_data_files = 0;
	tap.total_checksums = 0;
	tap.total_checksums_good = 0;
	tap.optimized_files = 0;
	tap.total_read_errors = 0;
	tap.fdate = 0;
	tap.taptime = 0;
	tap.version = 0;
	tap.bootable = 0;
	tap.changed = 0;
	tap.crc = 0;
	tap.cbmcrc = 0;
	tap.cbmid = 0;
	tap.tst_hd = 0;
	tap.tst_rc = 0;
	tap.tst_op = 0;
	tap.tst_cs = 0;
	tap.tst_rd = 0;

	reset_database();
	reset_prg_database();
}

/*
 * Unallocate tap, crc_table and database
 */

static void cleanup_main(void)
{
	unload_tap();
	free_crc_table();

	/* deallocate ram from file database */
	destroy_database();
}

/*
 * Get exe path from argv[0]...
 */

static int get_exedir(char *argv0)
{
	char *ret;

#ifdef WIN32
	/* First check if argv0 fits inside our buffer */
	if (strlen(argv0) > MAXPATH - 1)
		return FALSE;

	/* It's now safe to copy inside the buffer (padding with nulls!) */
	strncpy(exedir, argv0, MAXPATH);

	/* When run at the console argv[0] is simply "tapclean" or "tapclean.exe" */

	/* Note: we do this instead of using getcwd() because getcwd does not give
	 * the exe's directory when dragging and dropping a tap file to the program
	 * icon using windows explorer.
	 */
	if (stricmp(exedir, "tapclean") != 0 && stricmp(exedir, "tapclean.exe") != 0) {
		int i;

		/* Clip to leave path only */
		for (i = strlen(exedir) - 1; i > 0 && exedir[i] != SLASH; i--)
			;
	
		if (exedir[i] == SLASH)
			exedir[i + 1] = '\0';   

		return TRUE;
	}
#endif

	ret = (char *)getcwd(exedir, MAXPATH - 2);
	if (ret == NULL)
		return FALSE;

	exedir[strlen(exedir)] = SLASH;
	exedir[strlen(exedir)] = '\0';

	return TRUE;
}

/*
 * make a copy of the loader table ft[] so we can revert back to it after any changes.
 * the copy is globally available in ft_org[]
 */

static void copy_loader_table(void)
{
	int i;

	for (i = 0; ft[i].tp != 666; i++) {
		strcpy(ft_org[i].name, ft[i].name);
		ft_org[i].en = ft[i].en;
		ft_org[i].tp = ft[i].tp;
		ft_org[i].sp = ft[i].sp;
		ft_org[i].mp = ft[i].mp;
		ft_org[i].lp = ft[i].lp;
		ft_org[i].pv = ft[i].pv;
		ft_org[i].sv = ft[i].sv;
		ft_org[i].pmin = ft[i].pmin;
		ft_org[i].pmax = ft[i].pmax;
		ft_org[i].has_cs = ft[i].has_cs;
	}
}

/*
 * reset loader table to all original values, ONLY call this if a call to
 * copy_loader_table() has been made.
 */

static void reset_loader_table(void)
{
	int i;

	for (i = 0; ft[i].tp != 666; i++) {
		strcpy(ft[i].name, ft_org[i].name);
		ft[i].en = ft_org[i].en;
		ft[i].tp = ft_org[i].tp;
		ft[i].sp = ft_org[i].sp;
		ft[i].mp = ft_org[i].mp;
		ft[i].lp = ft_org[i].lp;
		ft[i].pv = ft_org[i].pv;
		ft[i].sv = ft_org[i].sv;
		ft[i].pmin = ft_org[i].pmin;
		ft[i].pmax = ft_org[i].pmax;
		ft[i].has_cs = ft_org[i].has_cs;
	}
}

/*
 * Display usage
 */

static void display_usage(void)
{
	printf("\n\nUsage:\n\n");
	printf("tapclean [[option][parameter]]...\n");
	printf("example: tapclean -o giana_sisters.tap -tol 12\n\n");
	printf("options...\n\n");

	printf(" -t   [tap]     Test TAP.\n");
	printf(" -o   [tap]     Optimize TAP.\n");
	printf(" -b   [dir]     Batch test.\n");
	printf(" -au  [tap]     Convert TAP to Sun AU audio file (44kHz).\n");
	printf(" -wav [tap]     Convert TAP to Microsoft WAV audio file (44kHz).\n");
	printf(" -rs  [tap]     Corrects the 'size' field of a TAPs header.\n");
	printf(" -ct0 [tap]     Convert TAP to version 0 format.\n");
	printf(" -ct1 [tap]     Convert TAP to version 1 format.\n\n");

	printf(" -16            Commodore 16 tape.\n");
	printf(" -20            Commodore VIC 20 tape.\n");
	printf(" -64            Commodore 64 tape (default).\n");
      
	printf(" -boostclean    Raise cleaning threshold.\n");
	printf(" -debug         Allows detected files to overlap.\n");
	printf(" -docyberfault  Report Cyberload F3 bad checksums of $04.\n");
	printf(" -doprg         Create PRG files.\n");
	printf(" -extvisipatch  Extract Visiload loader patch files.\n");
	printf(" -incsubdirs    Make batch scan include subdirectories.\n");
	printf(" -list          List of supported scanners and options used by -no<loader>\n");
	printf(" -noaddpause    Dont add a pause to the file end after clean.\n");
	printf(" -noc64eof      C64 ROM scanner will not expect EOF markers.\n");
	printf(" -noid          Disable scanning for only the 1st ID'd loader.\n");
	printf(" -no<loader>    Don't scan for this loader. Example: -nocyber.\n");
	printf(" -do<loader>    Scan only for this loader.\n");
	printf(" -ntsc          NTSC timing.\n");
	printf(" -pal           PAL timing (default).\n");
	printf(" -prgunite      Connect neighbouring PRG's into a single file.\n");
	printf(" -sine          Make audio converter use sine waves.\n");
	printf(" -sortbycrc     Batch scan sorts report by cbmcrc values.\n");
	printf(" -skewadapt     Use skewed pulse adapting bit reader.\n");
	printf(" -tol [0-15]    Set pulsewidth read tolerance, default = 10.\n");
}

/*
 * Display scanner list
 */

static void display_scanners(void)
{
	int i,l;
	printf("\nList of supported scanners and their -no<loader>/-do<loader> parameter names.\n\n");
	l=sizeof(ldrswt)/sizeof(*ldrswt);
	for(i=0;i<l;i++)
		printf(" %-24s  -no%-12s  -do%-12s\n",ldrswt[i].desc,ldrswt[i].par,ldrswt[i].par );
}

/* noall */
static void set_noall(void)
{
	int i,l;
	l=sizeof(ldrswt)/sizeof(*ldrswt);
	ldrswt[0].state = FALSE;
	for(i=1;i<l;i++)
		ldrswt[i].state = TRUE;
}

/*
 * Process options
 */

static void process_options(int argc, char **argv)
{
	int i;
	int excludeflag = 1;

	int jj, sl;	/* counter and amount of loader switches */
	sl = sizeof(ldrswt)/sizeof(*ldrswt);

	for (i = 0; i < argc; i++) {
		if (strcmp(argv[i], "-tol") == 0) {		/* flag = set tolerance */
			if (argv[i + 1] != NULL) {
				tol = atoi(argv[i + 1]) + 1;	/* 1 = zero tolerance (-tol 0) */
				if (tol < 0 || tol > MAXTOL) {
					tol = DEFTOL;
					printf("\n\nTolerance parameter out of range, using default (= 10).");
				}
			} else
				printf("\n\nTolerance parameter missing, using default (= %d).", DEFTOL);
		}

		if (strcmp(argv[i], "-debug") == 0)
			debug = TRUE;
		if (strcmp(argv[i], "-noid") == 0)
			noid = TRUE;
		if (strcmp(argv[i], "-16") == 0) {
			c16 = TRUE;
			c64 = FALSE;
		}
		if (strcmp(argv[i], "-20") == 0) {
			c20 = TRUE;
			c64 = FALSE;
		}
		if (strcmp(argv[i], "-64") == 0)
			c64 = TRUE;
		if (strcmp(argv[i], "-pal") == 0)
			pal = TRUE;
		if (strcmp(argv[i], "-ntsc") == 0)
			ntsc = TRUE;
		if (strcmp(argv[i], "-noc64eof") == 0)
			noc64eof = TRUE;
		if (strcmp(argv[i], "-docyberfault") == 0)
			docyberfault = TRUE;
		if (strcmp(argv[i], "-boostclean") == 0)
			boostclean = TRUE;
		if (strcmp(argv[i], "-noaddpause") == 0)
			noaddpause = TRUE;
		if (strcmp(argv[i], "-sine") == 0)
			sine = TRUE;
		if (strcmp(argv[i], "-prgunite") == 0) {
			prgunite = TRUE;
			doprg = TRUE;
		}
		if (strcmp(argv[i], "-doprg") == 0)
			doprg = TRUE;
		if (strcmp(argv[i], "-extvisipatch") == 0)
			extvisipatch = TRUE;
		if (strcmp(argv[i], "-incsubdirs") == 0)
			incsubdirs = TRUE;
		if (strcmp(argv[i], "-list") == 0)
		{
			display_scanners();
			exit(0);
		}
		if (strcmp(argv[i], "-sortbycrc") == 0)
			sortbycrc = TRUE;
		if (strcmp(argv[i], "-ec") == 0)
			exportcyberloaders = TRUE;
		if (strcmp(argv[i], "-skewadapt") == 0)
			skewadapt = TRUE;

		/* process all -no<loader> */
		if (strncmp(argv[i], "-no", 3) == 0) {
			for(jj = 0; jj < sl; jj++) {
				if (strcmp(argv[i]+3, ldrswt[jj].par) ==  0) {
					if (excludeflag == 1) {
						printf("\nExcluded scanners:\n\n");
						/* Print above message only once */
						excludeflag = 0;
					} else if (excludeflag == 2) {
						printf("You cannot mix -no<loader> and -do<loader>\n");
						exit(1);
					}
					ldrswt[jj].state = TRUE;
					printf(" %s\n", ldrswt[jj].desc);
				}
			}
		}


		/* process all -do<loader> */

		if (strncmp(argv[i], "-do", 3) == 0) {
			for(jj = 0; jj < sl; jj++) {
				if (strcmp(argv[i]+3, ldrswt[jj].par) ==  0) {
					if (excludeflag == 1) {
						printf("\nIncluded scanners:\n\n");
						set_noall();
						/* Print above message only once */
						excludeflag = 2;
					} else if (excludeflag == 0) {
						printf("You cannot mix -no<loader> and -do<loader>\n");
						exit(1);
					}
					ldrswt[jj].state = FALSE;
					printf(" +%s\n", ldrswt[jj].desc);
				}
			}
		}
	}

	printf("\nRead tolerance = %d", tol - 1);
}

/*
 * Choose CPU cycles based on computer type and PAL/NTSC
 */

static void handle_cps(void)
{
	printf("\n\nComputer type: ");

	if (c64 == TRUE) {
		printf("C64 ");
		if (pal == TRUE)
			cps = C64_PAL_CPS;
		else
			cps = C64_NTSC_CPS;
	} else if (c16 == TRUE) {
		printf("C16 ");
		/* Force CBM pulsewidths to the ones found in C16/+4 tapes */

		ft [CBM_HEAD].sp = 0x35;
		ft [CBM_HEAD].mp = 0x6A;
		ft [CBM_HEAD].lp = 0xD4;

		ft [CBM_DATA].sp = 0x35;
		ft [CBM_DATA].mp = 0x6A;
		ft [CBM_DATA].lp = 0xD4;

		if (pal == TRUE)
			cps = C16_PAL_CPS;
		else
			cps = C16_NTSC_CPS;
	} else if (c20 == TRUE) {
		printf("VIC20 ");

		/* Force CBM pulsewidths to the ones found in VIC20 tapes */
		ft [CBM_HEAD].sp = 0x2B;
		ft [CBM_HEAD].mp = 0x3F;
		ft [CBM_HEAD].lp = 0x53;

		ft [CBM_DATA].sp = 0x2B;
		ft [CBM_DATA].mp = 0x3F;
		ft [CBM_DATA].lp = 0x53;

		if (pal == TRUE)
			cps = VIC20_PAL_CPS;
		else
			cps = VIC20_NTSC_CPS;
	}

	if (pal == TRUE)
		printf("PAL ");
	else
		printf("NTSC ");

	printf("(%ld Hz)\n", cps);
}

/*
 * Search the tap for all known (and enabled) file formats.
 *
 * Results are stored in the 'blk' database (an array of structs).
 *
 * If global variable 'quiet' is set (1), the scanners will not print a
 * "Scanning: xxxxxxx" string. This simply reduces text output and I prefer this
 * when optimizing (Search is called quite frequently).
 *
 * Note: This function calls all 'loadername_search' routines and as a result it
 *       fills out only about half of the fields in the blk[] database, they are...
 *
 *       lt, p1, p2, p3, p4, xi (ie, all fields supported by 'addblockdef()').
 *
 *       the remaining fields (below) are filled out by 'describe_blocks()' which
 *       calls all 'loadername_decribe()' routines for the particular format (lt).
 *
 *       cs, ce, cx, rd_err, crc, cs_exp, cs_act, pilot_len, trail_len, ok.
 */

static void search_tap(void)
{
	long i;

	dbase_is_full = FALSE;	/* enable the "database full" warning. */
				/* note: addblockdef sets it 1 when full. */
 
	msgout("\nScanning...");

	if (tap.changed) {
		reset_database();

		/* initialize the read error table */

		for (i = 0; i < NUM_READ_ERRORS; i++)
			read_errors[i] = 0;


		/* CALL THE SCANNERS!... */

		pause_search();		/* pauses and CBM files always get searched for...  */
		cbm_search();

		/* Note : if cbm parts are found then this call (cbm_search) will create copies
		 * of the header and data parts in 'cbm_header[]' and 'cbm_program[]'.
		 * Also a crc32 of the data block is recorded in 'tap.cbmcrc'.
		 */

		/* try and id any loader stored in cbm_program[]... */

		tap.cbmid = idloader(tap.cbmcrc);

		if (!quiet) {
			sprintf(lin, "\n  Loader ID: %s.\n", knam[tap.cbmid]);
			msgout(lin);
		}

		if (noid == FALSE) {	/* scanning shortcuts enabled?  */
			if (tap.cbmid == LID_T250 	&& ldrswt[noturbo].state == FALSE && !dbase_is_full && !aborted)
				turbotape_search();

			if (tap.cbmid == LID_FREE 	&& ldrswt[nofree ].state== FALSE && !dbase_is_full && !aborted)
				freeload_search();

			if (tap.cbmid == LID_ODE 	&& ldrswt[noode ].state== FALSE && !dbase_is_full && !aborted)
				odeload_search();

			if (tap.cbmid == LID_NOVA 	&& ldrswt[nonova ].state== FALSE && !dbase_is_full && !aborted) {
				nova_spc_search();
				nova_search();
			}

			if (tap.cbmid == LID_BLEEP 	&& ldrswt[nobleep].state == FALSE && !dbase_is_full && !aborted) {
				bleep_search();
				bleep_spc_search();
			}

			if (tap.cbmid == LID_OCEAN 	&& ldrswt[noocean].state == FALSE && !dbase_is_full && !aborted)
				ocean_search();

			if (tap.cbmid == LID_CYBER 	&& ldrswt[nocyber].state == FALSE &&!dbase_is_full) {
				cyberload_f1_search();
				cyberload_f2_search();
				cyberload_f3_search();
				cyberload_f4_search();
			}

			if (tap.cbmid == LID_USG 	&& ldrswt[nousgold	].state == FALSE && !dbase_is_full && !aborted)
				usgold_search();

			if (tap.cbmid == LID_ACE 	&& ldrswt[noaces 	].state == FALSE && !dbase_is_full && !aborted)
				aces_search();

			if (tap.cbmid == LID_MIC 	&& ldrswt[nomicro	].state == FALSE && !dbase_is_full && !aborted)
				micro_search();

			if (tap.cbmid == LID_RAST 	&& ldrswt[noraster	].state == FALSE && !dbase_is_full && !aborted)
				raster_search();

			if (tap.cbmid == LID_CHR 	&& ldrswt[nochr		].state == FALSE && !dbase_is_full)
				chr_search();

			if (tap.cbmid == LID_BURN 	&& ldrswt[noburner 	].state == FALSE && !dbase_is_full && !aborted)
				burner_search();

			/* if it's a visiload then choose correct type now... */

			if (tap.cbmid == LID_VIS1)
				visi_type = VISI_T1;

			if (tap.cbmid == LID_VIS2)
				visi_type = VISI_T2;

			if (tap.cbmid == LID_VIS3)
				visi_type = VISI_T3;

			if (tap.cbmid == LID_VIS4)
				visi_type = VISI_T4;

			if (tap.cbmid == LID_VIS1 || tap.cbmid == LID_VIS2 || tap.cbmid == LID_VIS3 || tap.cbmid == LID_VIS4) {
				if (ldrswt[novisi].state == FALSE && !dbase_is_full && !aborted)
					visiload_search();
			}

			if (tap.cbmid == LID_WILD 	&& ldrswt[nowild	].state == FALSE && !dbase_is_full && !aborted)
				wild_search();

			if (tap.cbmid == LID_HIT 	&& ldrswt[nohit     ].state == FALSE && !dbase_is_full && !aborted)
				hitload_search();

			if (tap.cbmid == LID_RACK 	&& ldrswt[norackit	].state	== FALSE && !dbase_is_full && !aborted)
				rackit_search();

			if (tap.cbmid == LID_SPAV 	&& ldrswt[nospav	].state == FALSE && !dbase_is_full && !aborted)
				superpav_search();

			if (tap.cbmid == LID_ANI 	&& ldrswt[noanirog	].state == FALSE && !dbase_is_full && !aborted) {
				anirog_search();
				freeload_search();
			}

			if (tap.cbmid == LID_SUPER 	&& ldrswt[nosuper	].state == FALSE && !dbase_is_full && !aborted)
				supertape_search();

			if (tap.cbmid == LID_FIRE 	&& ldrswt[nofire	].state == FALSE && !dbase_is_full && !aborted)
				firebird_search();

			if (tap.cbmid == LID_PAV 	&& ldrswt[nopav		].state == FALSE && !dbase_is_full && !aborted)
				pav_search();

			if (tap.cbmid == LID_IK 	&& ldrswt[noik		].state == FALSE && !dbase_is_full && !aborted)
				ik_search();

			if (tap.cbmid == LID_FLASH 	&& ldrswt[noflash	].state == FALSE && !dbase_is_full && !aborted)
				flashload_search();

			if (tap.cbmid == LID_VIRG 	&& ldrswt[novirgin	].state == FALSE && !dbase_is_full && !aborted)
				virgin_search();

			if (tap.cbmid == LID_HTEC 	&& ldrswt[nohitec	].state == FALSE && !dbase_is_full && !aborted)
				hitec_search();

			if (tap.cbmid == LID_OCNEW1T1	&& ldrswt[nooceannew1t1	].state == FALSE && !dbase_is_full && !aborted)
				oceannew1t1_search();

			if (tap.cbmid == LID_OCNEW1T2	&& ldrswt[nooceannew1t2	].state == FALSE && !dbase_is_full && !aborted)
				oceannew1t2_search();

			if (tap.cbmid == LID_OCNEW2	&& ldrswt[nooceannew2	].state == FALSE && !dbase_is_full && !aborted)
				oceannew2_search();

			if (tap.cbmid == LID_SNAKE	&& ldrswt[nosnake50	].state == FALSE && !dbase_is_full && !aborted) {
				snakeload50t1_search();
				snakeload50t2_search();
			}

			if (tap.cbmid == LID_SNAKE	&& ldrswt[nosnake51	].state == FALSE && !dbase_is_full && !aborted)
				snakeload51_search();

			if (tap.cbmid == LID_ATLAN	&& ldrswt[noatlantis	].state == FALSE && !dbase_is_full && !aborted)
				atlantis_search();

			if (tap.cbmid == LID_AUDIOGENIC	&& ldrswt[noaudiogenic	].state == FALSE && !dbase_is_full && !aborted)
				audiogenic_search();

			if (tap.cbmid == LID_CULT	&& ldrswt[nocult	].state == FALSE && !dbase_is_full && !aborted)
				cult_search();

			if (tap.cbmid == LID_ACCOLADE	&& ldrswt[noaccolade	].state == FALSE && !dbase_is_full && !aborted)
				accolade_search();

			if (tap.cbmid == LID_RAINBOWARTS	&& ldrswt[norainbowf1	].state == FALSE && !dbase_is_full && !aborted)
				rainbowf1_search();

			if (tap.cbmid == LID_RAINBOWARTS	&& ldrswt[norainbowf2	].state == FALSE && !dbase_is_full && !aborted)
				rainbowf2_search();

			if (tap.cbmid == LID_BURNERVAR	&& ldrswt[noburnervar	].state == FALSE && !dbase_is_full && !aborted)
				burnervar_search();

			if (tap.cbmid == LID_OCNEW4	&& ldrswt[nooceannew2	].state == FALSE && !dbase_is_full && !aborted)
				oceannew4_search();

			if (tap.cbmid == LID_108DE0A5	&& ldrswt[no108DE0A5	].state == FALSE && !dbase_is_full && !aborted)
				_108DE0A5_search();

			if (tap.cbmid == LID_FREE_SLOW	&& ldrswt[nofrslow	].state == FALSE && !dbase_is_full && !aborted)
				freeslow_search();

			if (tap.cbmid == LID_GOFORGOLD	&& ldrswt[nogoforgold	].state == FALSE && !dbase_is_full && !aborted)
				goforgold_search();

			if (tap.cbmid == LID_FASTEVIL	&& ldrswt[nofastevil	].state == FALSE && !dbase_is_full && !aborted)
				fastevil_search();

			if (tap.cbmid == LID_FFTAPE	&& ldrswt[nofftape	].state == FALSE && !dbase_is_full && !aborted)
				fftape_search();

			if (tap.cbmid == LID_TESTAPE	&& ldrswt[notestape	].state == FALSE && !dbase_is_full && !aborted)
				testape_search();

			/*
			 * todo : TURRICAN
			 * todo : SEUCK
			 * todo : JETLOAD
			 * todo : TENGEN
			 */
		}      

		/* Scan the lot.. (if shortcuts are disabled or no loader ID was found) */

		if ((noid == FALSE && tap.cbmid == 0) || (noid == TRUE)) {
			if (ldrswt[noturbo	].state == FALSE && !dbase_is_full && !aborted)
				turbotape_search();

			if (ldrswt[nofree	].state == FALSE && !dbase_is_full && !aborted)
				freeload_search();

			if (ldrswt[noode	].state == FALSE && !dbase_is_full && !aborted)
				odeload_search();

			if (ldrswt[nocult	].state == FALSE && !dbase_is_full && !aborted)
				cult_search();

			/*
			 * Comes here to avoid ocean misdetections.
			 * Snakeload is a 'safer' scanner than ocean (the
			 * confidence level with which it is acknowledged is
			 * higher than ocean).
			 */

			if (ldrswt[nosnake50	].state == FALSE && !dbase_is_full && !aborted) {
				snakeload50t1_search();
				snakeload50t2_search();
			}

			if (ldrswt[nosnake51	].state == FALSE && !dbase_is_full && !aborted)
				snakeload51_search();

			if (ldrswt[nonova	].state == FALSE && !dbase_is_full && !aborted) {
				nova_spc_search();
				nova_search();
			}

			if (ldrswt[nobleep	].state == FALSE && !dbase_is_full && !aborted) {
				bleep_search();
				bleep_spc_search();
			}

			if (ldrswt[noocean	].state == FALSE && !dbase_is_full && !aborted)
				ocean_search();

			if (ldrswt[nocyber	].state == FALSE && !dbase_is_full && !aborted) {
				cyberload_f1_search();
				cyberload_f2_search();
				cyberload_f3_search();
				cyberload_f4_search();
			}

			if (ldrswt[nousgold	].state == FALSE && !dbase_is_full && !aborted)
				usgold_search();

			if (ldrswt[noaces	].state == FALSE && !dbase_is_full && !aborted)
				aces_search();

			if (ldrswt[nomicro	].state == FALSE && !dbase_is_full && !aborted)
				micro_search();

			if (ldrswt[noraster	].state == FALSE && !dbase_is_full && !aborted)
				raster_search();

			if (ldrswt[nochr	].state == FALSE && !dbase_is_full && !aborted)
				chr_search();

			if (ldrswt[noburner	].state == FALSE && !dbase_is_full && !aborted)
				burner_search();

			if (ldrswt[novisi	].state == FALSE && !dbase_is_full && !aborted)
				visiload_search();

			if (ldrswt[nowild	].state == FALSE && !dbase_is_full && !aborted)
				wild_search();

			if (ldrswt[nohit	].state == FALSE && !dbase_is_full && !aborted)
				hitload_search();

			if (ldrswt[norackit	].state == FALSE && !dbase_is_full && !aborted)
				rackit_search();

			if (ldrswt[nospav	].state == FALSE && !dbase_is_full && !aborted)
				superpav_search();

			if (ldrswt[noanirog	].state == FALSE && !dbase_is_full && !aborted)
				anirog_search();

			if (ldrswt[nosuper	].state == FALSE && !dbase_is_full && !aborted)
				supertape_search();

			if (ldrswt[nofire	].state == FALSE && !dbase_is_full && !aborted)
				firebird_search();

			if (ldrswt[nopav	].state == FALSE && !dbase_is_full && !aborted)
				pav_search();

			if (ldrswt[noik		].state == FALSE && !dbase_is_full && !aborted)
				ik_search();

			if (ldrswt[noturr	].state == FALSE && !dbase_is_full && !aborted)
				turrican_search();

			if (ldrswt[noseuck	].state == FALSE && !dbase_is_full && !aborted)
				seuck1_search();

			if (ldrswt[nojet	].state == FALSE && !dbase_is_full && !aborted)
				jetload_search();

			if (ldrswt[noflash	].state == FALSE && !dbase_is_full && !aborted)
				flashload_search();

			if (ldrswt[novirgin	].state == FALSE && !dbase_is_full && !aborted)
				virgin_search();

			if (ldrswt[nohitec	].state == FALSE && !dbase_is_full && !aborted)
				hitec_search();

			if (ldrswt[notdif2	].state == FALSE && !dbase_is_full && !aborted) /* check f2 first (luigi) */
				tdif2_search();

			if (ldrswt[notdif1	].state == FALSE && !dbase_is_full && !aborted)
				tdi_search();

			if (ldrswt[nooceannew1t1].state == FALSE && !dbase_is_full && !aborted)
				oceannew1t1_search();

			if (ldrswt[nooceannew1t2].state == FALSE && !dbase_is_full && !aborted)
				oceannew1t2_search();

			if (ldrswt[nooceannew2	].state == FALSE && !dbase_is_full && !aborted)
				oceannew2_search();

			if (ldrswt[noatlantis	].state == FALSE && !dbase_is_full && !aborted)
				atlantis_search();

			if (ldrswt[nopalacef1	].state == FALSE && !dbase_is_full && !aborted)
				palacef1_search();

			if (ldrswt[nopalacef2	].state == FALSE && !dbase_is_full && !aborted)
				palacef2_search();

			if (ldrswt[noenigma	].state == FALSE && !dbase_is_full && !aborted)
				enigma_search();

			if (ldrswt[noaudiogenic	].state == FALSE && !dbase_is_full && !aborted)
				audiogenic_search();

			if (ldrswt[noaliensy	].state == FALSE && !dbase_is_full && !aborted)
				aliensyndrome_search();

			if (ldrswt[noaccolade	].state == FALSE && !dbase_is_full && !aborted)
				accolade_search();

			if (ldrswt[noalterwg	].state	== FALSE && !dbase_is_full && !aborted)
				alternativewg_search();

			if (ldrswt[norainbowf1	].state	== FALSE && !dbase_is_full && !aborted)
				rainbowf1_search();

			if (ldrswt[norainbowf2	].state	== FALSE && !dbase_is_full && !aborted)
				rainbowf2_search();

			if (ldrswt[notrilogic	].state	== FALSE && !dbase_is_full && !aborted)
				trilogic_search();

			if (ldrswt[noburnervar	].state	== FALSE && !dbase_is_full && !aborted)
				burnervar_search();

			if (ldrswt[nooceannew4	].state	== FALSE && !dbase_is_full && !aborted)
				oceannew4_search();

			if (ldrswt[nobiturbo	].state == FALSE && !dbase_is_full && !aborted)
				biturbo_search();

			if (ldrswt[no108DE0A5	].state == FALSE && !dbase_is_full && !aborted)
				_108DE0A5_search();

			if (ldrswt[noar		].state == FALSE && !dbase_is_full && !aborted)
				ar_search();

			if (ldrswt[noashdave	].state == FALSE && !dbase_is_full && !aborted)
				ashdave_search();

			if (ldrswt[nofrslow	].state == FALSE && !dbase_is_full && !aborted)
				freeslow_search();

			/*
			 * Do not add the following ones because they should only be looked for when 
			 * their signature is found in CBM Data block.
			 */

			//if (ldrswt[notestape	].state == FALSE && !dbase_is_full && !aborted)
			//	testape_search();

			//if (ldrswt[nofftape		].state == FALSE && !dbase_is_full && !aborted)
			//	fftape_search();

			/*
			 * Do not add the following ones until additonal games using these formats 
			 * are found.
			 * For the time being having these active would just slow down testing and
			 * cleaning other tapes. Besides, these are looked for when their signature 
			 * is found in CBM Data block.
			 */

			//if (ldrswt[nogoforgold	].state == FALSE && !dbase_is_full && !aborted)
			//	goforgold_search();

			//if (ldrswt[nofastevil		].state == FALSE && !dbase_is_full && !aborted)
			//	fastevil_search();
		}

		sort_blocks();	/* sort the blocks into order of appearance */
		scan_gaps();	/* add any gaps to the block list */
		sort_blocks();	/* sort the blocks into order of appearance */

		tap.changed = 0;	/* important to clear this. */

		if (quiet)
			msgout("  Done.");
	} else
		msgout("  Done.");
}

/*
 * Write a description of a GAP file to global buffer 'info' for inclusion in
 * the report.
 */

static void gap_describe(int row)
{
	if (blk[row]->xi > 1)
		sprintf(lin, "\n - Length = %d pulses", blk[row]->xi);
	else
		sprintf(lin, "\n - Length = %d pulse", blk[row]->xi);

	strcpy(info, lin);
}

/*
 * Pass this function a valid row number (i) from the file database (blk) and
 * it calls the describe function for that file format which will decode
 * any data in the block and fill in all missing information for that file.
 *
 * Note: Any "additional" (unstored in the database) text info will be available
 * in the global buffer 'info' (this could be improved).
 */

static void describe_file(int row)
{
	int t;
	t = blk[row]->lt;

	switch(t) {
		case GAP:		gap_describe(row);
					break;
		case PAUSE:		pause_describe(row);
					break;
		case CBM_HEAD:		cbm_describe(row);
					break;
		case CBM_DATA:		cbm_describe(row);
					break;
		case USGOLD:		usgold_describe(row);
					break;
		case TT_HEAD:		turbotape_describe(row);
					break;
		case TT_DATA:		turbotape_describe(row);
					break;
		case FREE:		freeload_describe(row);
					break;
		case ODELOAD:		odeload_describe(row);
					break;
		case CULT:		cult_describe(row);
					break;
		case CHR_T1:		chr_describe(row);
					break;
		case CHR_T2:		chr_describe(row);
					break;
		case CHR_T3:		chr_describe(row);
					break;
		case NOVA:		nova_describe(row);
					break;
		case NOVA_SPC:		nova_spc_describe(row);
					break;
		case WILD:		wild_describe(row);
					break;
		case WILD_STOP:		wild_describe(row);
					break;
		case ACES:		aces_describe(row);
					break;
		case OCEAN_F1:		ocean_describe(row);
					break;
		case OCEAN_F2:		ocean_describe(row);
					break;
		case OCEAN_F3:		ocean_describe(row);
					break;
		case RASTER:		raster_describe(row);
					break;
		case VISI_T1:		visiload_describe(row);
					break;
		case VISI_T2:		visiload_describe(row);
					break;
		case VISI_T3:		visiload_describe(row);
					break;
		case VISI_T4:		visiload_describe(row);
					break;
		case CYBER_F1:		cyberload_f1_describe(row);
					break;
		case CYBER_F2:		cyberload_f2_describe(row);
					break;
		case CYBER_F3:		cyberload_f3_describe(row);
					break;
		case CYBER_F4_1:	cyberload_f4_describe(row);
					break;
		case CYBER_F4_2:	cyberload_f4_describe(row);
					break;
		case CYBER_F4_3:	cyberload_f4_describe(row);
					break;
		case BLEEP:		bleep_describe(row);
					break;
		case BLEEP_TRIG:	bleep_describe(row);
					break;
		case BLEEP_SPC:		bleep_spc_describe(row);
					break;
		case HITLOAD:		hitload_describe(row);
					break;
		case MICROLOAD:		micro_describe(row);
					break;
		case BURNER:		burner_describe(row);
					break;
		case RACKIT:		rackit_describe(row);
					break;
		case SPAV1_HD:		superpav_describe(row);
					break;
		case SPAV2_HD:		superpav_describe(row);
					break;
		case SPAV1:		superpav_describe(row);
					break;
		case SPAV2:		superpav_describe(row);
					break;
		case VIRGIN:		virgin_describe(row);
					break;
		case HITEC:		hitec_describe(row);
					break;
		case ANIROG:		anirog_describe(row);
					break;
		case SUPERTAPE_HEAD:	supertape_describe(row);
					break;
		case SUPERTAPE_DATA:	supertape_describe(row);
					break;
		case PAV:		pav_describe(row);
					break;
		case IK:		ik_describe(row);
					break;
		case FBIRD1:		firebird_describe(row);
					break;
		case FBIRD2:		firebird_describe(row);
					break;
		case TURR_HEAD:		turrican_describe(row);
					break;
		case TURR_DATA:		turrican_describe(row);
					break;
		case SEUCK_L2:		seuck1_describe(row);
					break;
		case SEUCK_HEAD:	seuck1_describe(row);
					break;
		case SEUCK_DATA:	seuck1_describe(row);
					break;
		case SEUCK_TRIG:	seuck1_describe(row);
					break;
		case SEUCK_GAME:	seuck1_describe(row);
					break;
		case JET:		jetload_describe(row);
					break;
		case FLASH:		flashload_describe(row);
					break;
		case TDI_F1:		tdi_describe(row);
					break;
		case OCNEW1T1:		oceannew1t1_describe(row);
					break;
		case OCNEW1T2:		oceannew1t2_describe(row);
					break;
		case ATLAN:		atlantis_describe(row);
					break;
		case SNAKE51:		snakeload51_describe(row);
					break;
		case SNAKE50T1:		snakeload50t1_describe(row);
					break;
		case SNAKE50T2:		snakeload50t2_describe(row);
					break;
		case PAL_F1:		palacef1_describe(row);
					break;
		case PAL_F2:		palacef2_describe(row);
					break;
		case OCNEW2:		oceannew2_describe(row);
					break;
		case ENIGMA:		enigma_describe(row);
					break;
		case AUDIOGENIC:	audiogenic_describe(row);
					break;
		case ALIENSY:		aliensyndrome_describe(row);
					break;
		case ACCOLADE:		accolade_describe(row);
					break;
		case ALTERWG:		alternativewg_describe(row);
					break;
		case RAINBOWARTSF1:	rainbowf1_describe(row);
					break;
		case RAINBOWARTSF2:	rainbowf2_describe(row);
					break;
		case TRILOGIC:		trilogic_describe(row);
					break;
		case BURNERVAR:		burnervar_describe(row);
					break;
		case OCNEW4:		oceannew4_describe(row);
					break;
		case TDI_F2:		tdif2_describe(row);
					break;
		case BITURBO:		biturbo_describe(row);
					break;
		case _108DE0A5:		_108DE0A5_describe(row);
					break;
		case ACTIONREPLAY_HDR:	ar_describe_hdr(row);
					break;
		case ACTIONREPLAY_TURBO:
		case ACTIONREPLAY_STURBO:
					ar_describe_data(row);
					break;
		case ASHDAVE:		ashdave_describe(row);
					break;
		case FREE_SLOW:		freeslow_describe(row);
					break;
		case GOFORGOLD:		goforgold_describe(row);
					break;
		case FASTEVIL:		fastevil_describe(row);
					break;
		case FFTAPE:		fftape_describe(row);
					break;
		case TESTAPE:		testape_describe(row);
					break;
	}
}

/*
 * Describes each file in the database, this calls the loadername_describe()
 * function for the files type which fills in all missing information and
 * decodes each file.
 */

static void describe_blocks(void)
{
	int i, t, re;

	tap.total_read_errors = 0;

	for (i = 0; blk[i]->lt != 0; i++) {
		t = blk[i]->lt;

		describe_file(i);

		/* get generic info that all data blocks have... */

		if (t > PAUSE) {

			/* make crc32 if the block has been data extracted. */

			if (blk[i]->dd != NULL)
				blk[i]->crc = compute_crc32(blk[i]->dd, blk[i]->cx);

			re = blk[i]->rd_err;
			tap.total_read_errors += re;
		}
	}
}

/*
 * Save buffer tap.tmem[] to a named file.
 * Return 1 on success, 0 on error.
 */

static int save_tap(char *name)
{
	FILE *fp;

	fp = fopen(name, "w+b");
	if (fp == NULL || tap.tmem == NULL)
		return 0;

	fwrite(tap.tmem, tap.len, 1, fp);
	fclose(fp);
	return 1;
}

/*
 * Look at the TAP header and verify signature as C64 TAP.
 * Return 0 if ok else 1.
 */

static int check_signature(void)
{
	int i;

	/* copy 1st 12 chars, strncpy fucks it up somehow. */

	for (i = 0; i < 12; i++)
		lin[i] = tap.tmem[i];

	lin[i] = 0;

	if (strcmp(lin, "C64-TAPE-RAW") == 0)
		return 0;
	else
		return 1;
}

/*
 * Look at the TAP header and verify version, currently 0 and 1 are valid versions.
 * Sets 'version' variable and returns 0 on success, else returns 1.
 */

static int check_version(void)
{
	int b;

	b = tap.tmem[12];
	if (b == 0 || b == 1) {
		tap.version = b;
		return 0;
	} else
		return 1;
}

/*
 * Verify the TAP header 'size' field.
 * Returns 0 if ok, else 1.
 */

static int check_size(void)
{
	int sz;

	sz = tap.tmem[16] + (tap.tmem[17] << 8) + (tap.tmem[18] << 16) + (tap.tmem[19] << 24);
	if (sz == tap.len - 20)
		return 0;
	else
		return 1;
}

/*
 * Return the duration in seconds between p1 and p2.
 * p1 and p2 should be valid offsets within the scope of the TAP file.
 */

static float get_duration(int p1, int p2)
{
	/*long*/ int i;
	unsigned /*long*/ int zsum;
	double tot = 0;
	double p = (double)20000 / cps;
	float apr;

	for (i = p1; i < p2; i++) {

		/* handle normal pulses (non-zeroes) */

		if (tap.tmem[i] != 0)
			tot += ((double)(tap.tmem[i] * 8) / cps);

		/* handle v0 zeroes.. */

		if (tap.tmem[i] == 0 && tap.version == 0)
			tot += p;

		/* handle v1 zeroes.. */

		if (tap.tmem[i] == 0 && tap.version == 1) {
			zsum = tap.tmem[i + 1] + (tap.tmem[i + 2] << 8) + (tap.tmem[i + 3] << 16);
			tot += (double)zsum / cps;
			i += 3;
		}
	}

	apr = (float)tot;	/* approximate and return number of seconds. */
	return apr;
}

/*
 * Return the number of unique pulse widths in the TAP.
 * Note: Also fills tap.pst[256] array with distribution stats.
 */

static int get_pulse_stats(void)
{
	int i, tot, b;

	for(i = 0; i < 256; i++)	/* clear pulse table...  */
		tap.pst[i] = 0;

	/* create pulse table... */

	for (i = 20; i < tap.len; i++) {
		b = tap.tmem[i];
		if (b == 0 && tap.version == 1)
			i += 3;
		else
			tap.pst[b]++;
	}

	/* add up pulse types... */

	tot = 0;

	/* Note the start at 1 (pauses dont count) */

	for (i = 1; i < 256; i++) {
		if (tap.pst[i] != 0)
			tot++;
	}

	return tot;
}

/*
 * Count all file types found in the TAP and their quantities.
 * Also records the number of data files, checksummed data files and gaps in the TAP.
 */

static void get_file_stats(void)
{
	int i;

	for (i = 0; i < 256; i++)	/* init table */
		tap.fst[i] = 0;

	/* count all contained filetype occurences... */

	for (i = 0; blk[i]->lt != 0; i++)
		tap.fst[blk[i]->lt]++;

	tap.total_data_files = 0;
	tap.total_checksums = 0;
	tap.total_gaps = 0;

	/* for each file format in ft[]...  */

	for (i = 0; ft[i].sp != 666; i++) {
		if (tap.fst[i] != 0) {
			if (ft[i].has_cs == CSYES)
				tap.total_checksums += tap.fst[i];	/* count all available checksums. */
		}

		if (i > PAUSE)
			tap.total_data_files += tap.fst[i];		/* count data files */

		if (i == GAP)
			tap.total_gaps += tap.fst[i];			/* count gaps */
	}
}

/*
 * Print the human readable TAP report to a buffer.
 *
 * Note: this is done so I can send the info to both the screen and the report
 * without repeating the code.
 * Note: Call 'analyze()' before calling this!.
 */

static void print_results(char *buf)
{
	char szpass[2][5] = {"PASS", "FAIL"};
	char szok[2][5] = {"OK", "FAIL"};
	int min;
	float sec;

	sprintf(buf, "\n\n\nGENERAL INFO AND TEST RESULTS\n");
   
	sprintf(lin, "\nTAP Name    : %s", tap.path);
	strcat(buf, lin);

	sprintf(lin, "\nTAP Size    : %d bytes (%d kB)", tap.len, tap.len >> 10);
	strcat(buf, lin);
	sprintf(lin, "\nTAP Version : %d", tap.version);
	strcat(buf, lin);
	sprintf(lin, "\nRecognized  : %d%s", tap.detected_percent, "%%");
	strcat(buf, lin);
	sprintf(lin, "\nData Files  : %d", tap.total_data_files);
	strcat(buf, lin);
	sprintf(lin, "\nPauses      : %d", count_pauses());
	strcat(buf, lin);
	sprintf(lin, "\nGaps        : %d", tap.total_gaps);
	strcat(buf, lin);
	sprintf(lin, "\nMagic CRC32 : %08X", tap.crc);
	strcat(buf, lin);
	min = tap.taptime / 60;
	sec = tap.taptime - min * 60;
	sprintf(lin, "\nTAP Time    : %d:%.2f", min, sec);
	strcat(buf, lin);

	if (tap.bootable) {
		if (tap.bootable == 1)
			sprintf(lin, "\nBootable    : YES (1 part, name: %s)", pet2text(tmp, tap.cbmname));
		else
			sprintf(lin, "\nBootable    : YES (%d parts, 1st name: %s)", tap.bootable, pet2text(tmp, tap.cbmname));
		strcat(buf, lin);
	} else {
		sprintf(lin, "\nBootable    : NO");
		strcat(buf, lin);
	}

	sprintf(lin, "\nLoader ID   : %s", knam[tap.cbmid]);
	strcat(buf, lin);

	/* TEST RESULTS... */

	sprintf(lin, "\n");
	strcat(buf, lin);

	if (tap.tst_hd == 0 && tap.tst_rc == 0 && tap.tst_cs == 0 && tap.tst_rd == 0 && tap.tst_op == 0)
		sprintf(lin, "\nOverall Result    : PASS");
	else
		sprintf(lin, "\nOverall Result    : FAIL");

	strcat(buf, lin);

	sprintf(lin, "\n");
	strcat(buf, lin);
	sprintf(lin, "\nHeader test       : %s [Sig: %s] [Ver: %s] [Siz: %s]", szpass[tap.tst_hd], szok[tap.fsigcheck], szok[tap.fvercheck], szok[tap.fsizcheck]);
	strcat(buf, lin);
	sprintf(lin, "\nRecognition test  : %s [%d of %d bytes accounted for] [%d%s]", szpass[tap.tst_rc], tap.detected, tap.len - 20, tap.detected_percent,"%%");
	strcat(buf, lin); 
	sprintf(lin, "\nChecksum test     : %s [%d of %d checksummed files OK]", szpass[tap.tst_cs], tap.total_checksums_good, tap.total_checksums);
	strcat(buf, lin);
	sprintf(lin, "\nRead test         : %s [%d Errors]", szpass[tap.tst_rd], tap.total_read_errors);
	strcat(buf, lin);
	sprintf(lin, "\nOptimization test : %s [%d of %d files OK]", szpass[tap.tst_op], tap.optimized_files, tap.total_data_files);
	strcat(buf, lin);
	sprintf(lin, "\n");
	strcat(buf, lin);
}

/*
 * Note: This one prints out a fully interprets all file info and includes extra
 * text infos generated by xxxxx_describe functions.
 */

static void print_database(char *buf)
{
	int i;

	sprintf(buf, "\nFILE DATABASE\n");

	for (i = 0; blk[i]->lt != 0; i++) {
		sprintf(lin, "\n---------------------------------");
		strcat(buf, lin);
		sprintf(lin, "\nFile Type: %s", ft[blk[i]->lt].name);
		strcat(buf, lin);
		sprintf(lin, "\nLocation: $%04X -> $%04X -> $%04X -> $%04X", blk[i]->p1, blk[i]->p2, blk[i]->p3, blk[i]->p4);
		strcat(buf, lin);

		if (blk[i]->lt > PAUSE) {		/* info for data files only... */
			sprintf(lin, "\nLA: $%04X  EA: $%04X  SZ: %d", blk[i]->cs, blk[i]->ce, blk[i]->cx);
			strcat(buf, lin);

			if (blk[i]->fn != NULL) {	/* filename, if applicable */
				sprintf(lin, "\nFile Name: %s", blk[i]->fn);
				strcat(buf, lin);
			}

			sprintf(lin, "\nPilot/Trailer Size: %d/%d", blk[i]->pilot_len, blk[i]->trail_len);
			strcat(buf, lin);

			if (ft[blk[i]->lt].has_cs == TRUE) {	/* checkbytes, if applicable */
				sprintf(lin, "\nCheckbyte Actual/Expected: $%02X/$%02X, %s",
					blk[i]->cs_act, blk[i]->cs_exp, blk[i]->cs_act == blk[i]->cs_exp ? "PASS" : "FAIL");
				strcat(buf, lin);
			}

			sprintf(lin, "\nRead Errors: %d", blk[i]->rd_err);
			strcat(buf, lin);
			sprintf(lin, "\nUnoptimized Pulses: %d", count_unopt_pulses(i));
			strcat(buf, lin);
			sprintf(lin, "\nCRC32: %08X", blk[i]->crc);
			strcat(buf, lin);
		}

		strcpy(info, "");	/* clear 'info' ready to receive additional text */
		describe_file(i);
		strcat(buf, info);	/* add additional text */
		strcat(buf, "\n");
	}
}

/*
 * Print pulse stats to buffer 'buf'.
 * Note: Call 'analyze()' before calling this!.
 */

static void print_pulse_stats(char *buf)
{
	int i;

	sprintf(buf, "\nPULSE FREQUENCY TABLE\n");

	for (i = 1; i < 256; i++) {
		if (tap.pst[i] != 0) {
			sprintf(lin, "\n0x%02X (%d)", i, tap.pst[i]);
			strcat(buf, lin);
		}
	}
}

/*
 * Print file stats to buffer 'buf'.
 * Note: Call 'analyze()' before calling this!.
 */

static void print_file_stats(char *buf)
{
	int i;

	sprintf(buf, "\nFILE FREQUENCY TABLE\n");
 
	for (i = 0; ft[i].sp != 666; i++) {	/* for each file format in ft[]...  */
		if (tap.fst[i] != 0) {		/* list all found files and their frequency...  */
			sprintf(lin, "\n%s (%d)", ft[i].name, tap.fst[i]);
			strcat(buf, lin);
		}
	}
}


/**
 *	TAPClean entry point
 */

int main(int argc, char *argv[])
{
	int opnum;
	time_t t1, t2;
	
	char *opname;		/*!< a pointer to one of the following opnames */
	char opnames[][32] = {
		"No operation",
		"Testing", 
		"Optimizing",
		"Converting to v0",
		"Converting to v1", 
		"Fixing header size", 
		"Optimizing pauses",
		"Converting to au file",
		"Converting to wav file",
		"Batch scanning",
		"Creating info file"
	};

	/* Delete report and info files */
	deleteworkfiles();
         
	/* Get exe path from argv[0] */
	if (!get_exedir(argv[0]))
		return -1;

	/* Allocate database for files (not always needed, but still here) */
	if (!create_database())
		return -1;

	/* TBA */
	copy_loader_table();

	/* TBA */
	build_crc_table();

	printf("\n----------------------------------------------------------------------\n");
	printf(VERSION_STR" [Build: "__DATE__" by "BUILDER"]\n");
	printf("Based on Final TAP 2.76 Console - (C) 2001-2006 Subchrist Software\n");
	printf("----------------------------------------------------------------------\n");

	/* Note: options should be processed before actions! */
   
	if (argc == 1) {
		display_usage();
		printf("\n\n");
		cleanup_main();
		return 0;
	}

	process_options(argc, argv);
	handle_cps();
      
	/* PROCESS ACTIONS... */

	/** 
	 *	Just test a tap if no option is present, just a filename. 
	 *
	 * 	This allows for drag and drop in (Microsoft) explorer.
	 * 	First make sure the argument is not the -b option without
	 * 	arguments, or the -info option.
	 */

	if (argc == 2 && strcmp(argv[1], "-b") && strcmp(argv[1], "-info")) {
		if (load_tap(argv[1])) {
			printf("\n\nLoaded: %s", tap.name);
			printf("\nTesting...\n");
			time(&t1);
			if (analyze()) {
				report();
				printf("\n\nSaved: %s", tcreportname);
				time(&t2);
				time2str(t2 - t1, lin);
				printf("\nOperation completed in %s.", lin);
			}
		}
	} else {
		int i;

		for (i = 0; i < argc; i++) {
			opnum = 0;
			if (strcmp(argv[i], "-t") == 0)
				opnum = 1;	/* test */ 
			if (strcmp(argv[i], "-o") == 0)
				opnum = 2;	/* optimize */
			if (strcmp(argv[i], "-ct0") == 0)
				opnum = 3;	/* convert to v0 */
			if (strcmp(argv[i], "-ct1") == 0)
				opnum = 4;	/* convert to v1 */         
			if (strcmp(argv[i], "-rs") == 0)
				opnum = 5;	/* fix header size */
			if (strcmp(argv[i], "-po") == 0)
				opnum = 6;	/* pause optimize */
            
			if (strcmp(argv[i], "-au") == 0)
				opnum = 7;	/* convert to au */
			if (strcmp(argv[i], "-wav") == 0)
				opnum = 8;	/* convert to wav */
			if (strcmp(argv[i], "-b") == 0)
				opnum = 9;	/* batch scan */  
			if (strcmp(argv[i], "-info") == 0)
				opnum = 10;	/* create info file */
                  
			opname = opnames[opnum];
            
			/* This handles testing + any op that takes a tap, affects it and saves it... */

			if (opnum > 0 && opnum < 7) {
				if (argv[i + 1] != NULL) {
					if (load_tap(argv[i + 1])) {
						time(&t1);
						printf("\n\nLoaded: %s", tap.name);
						printf("\n%s...\n", opname);
                    
						if (analyze()) {
							switch(opnum) {
								case 2:	clean();
									break;
								case 3:	convert_to_v0();
									break;
								case 4:	convert_to_v1();
									break; 
								case 5:	fix_header_size(); 
									analyze();
									break; 
								case 6:	convert_to_v0(); 
									clip_ends(); 
									unify_pauses();
									convert_to_v1();   
									add_trailpause();
									break;
							}

							if (opnum > 1) {
								char *tapnamepos;

								strcpy(cleanedtapname, CLEANED_PREFIX);
								strcat(cleanedtapname, tap.name);
								change_file_extention(cleanedtapname, "tap", MAXPATH);
								save_tap(cleanedtapname);
								printf("\n\nSaved: %s", cleanedtapname);

								/* Change tap.path because the report refers to the cleaned tap */
								tapnamepos = strstr(tap.path, tap.name);
								if (tapnamepos)
									strncpy(tapnamepos, cleanedtapname, MAXPATH);
							}

							report();
							printf("\nSaved: %s", tcreportname);

							time(&t2);
							time2str(t2 - t1, lin);
							printf("\nOperation completed in %s.", lin);
						}
					}
				} else
					printf("\n\nMissing file name."); 
			}

			if (opnum == 7) {	/* flag = convert to au */
				if (argv[i + 1] != NULL) {
					if (load_tap(argv[i + 1])) {
						if (analyze()) {
							printf("\n\nLoaded: %s", tap.name);
							printf("\n%s...\n", opname);
							au_write(tap.tmem, tap.len, auoutname, sine);
							printf("\nSaved: %s", auoutname);
							msgout("\n");
						}
					}
				} else
					printf("\n\nMissing file name.");
			}
 
			if (opnum == 8) {		/* flag = convert to wav */
				if (argv[i + 1] != NULL) {
					if (load_tap(argv[i + 1])) {
						if (analyze()) {
							printf("\n\nLoaded: %s", tap.name);
							printf("\n%s...\n", opname);
							wav_write(tap.tmem, tap.len, wavoutname, sine);
							printf("\nSaved: %s", wavoutname);
							msgout("\n");
						}
					}
				} else
					printf("\n\nMissing file name.");
			}

			if (opnum == 9) {		/* flag = batch scan... */
				batchmode = TRUE;
				quiet = TRUE;

				if (argv[i + 1] != NULL) {
					printf("\n\nBatch Scanning: %s\n", argv[i + 1]);
					batchscan(argv[i + 1], incsubdirs, 1);
				} else {
					printf("\n\nMissing directory name, using current.");
					printf("\n\nBatch Scanning: %s\n", exedir);
					batchscan(exedir, incsubdirs, 1);
				}
			
				batchmode = FALSE;
				quiet = FALSE;
			}
                    
			/* flag = generate exe info file */

			if (opnum == 10) {
				FILE *fp;

				fp = fopen(tcinfoname, "w+t");
				if (fp != NULL) {
					printf("\n%s...\n", opname); 
					fprintf(fp, "%s", VERSION_STR);
					fclose(fp);
				}
			}
		}
	}

	printf("\n\n");
	cleanup_main();

	return 0;
}

/*
 * Read 1 pulse from the tap file offset at 'pos', decide whether it is a Bit0 or Bit1
 * according to the values in the parameters. (+/- global tolerance value 'tol')
 * lp = ideal long pulse width.
 * sp = ideal short pulse width.
 * tp = threshold pulse width (can be NA if unknown)
 *
 * Return (bit value) 0 or 1 on success, else -1 on failure.
 *
 * Note: Most formats can use this (and readttbyte()) for reading data, but some
 * (ie. cbmtape, pavloda, visiload,supertape etc) have custom readers held in their
 * own source files.
 */

int readttbit(int pos, int lp, int sp, int tp)
{
	int valid, v, b;

	if (skewadapt_enabled && tp != NA)
		return skewadapt_readttbit(pos, lp, sp, tp);

	if (pos < 20 || pos > tap.len - 1)	/* return error if out of bounds.. */
		return -1;

	if (is_pause_param(pos))		/* return error if offset is on a pause.. */
		return -1;

	valid = 0;
	b = tap.tmem[pos];

	if (tp != NA) {				/* exact threshold pulse IS available... */
		if (b < tp && b > sp - tol) {	/* its a SHORT (Bit0) pulse... */
			v = 0;
			valid += 1;
		}

		if (b > tp && b < lp + tol) {	/* its a LONG (Bit1) pulse... */
			v = 1;
			valid += 2;
		}

		if (b == tp)			/* its ON the threshold!... */
			valid += 4;
	} else {				/* threshold unknown? ...use midpoint method... */
		if (b > (sp - tol) && b < (sp + tol)) {	/* its a SHORT (Bit0) pulse...*/
			valid += 1;
			v = 0;
		}

		if (b > (lp - tol) && b < (lp + tol)) {	/* its a LONG (Bit1) pulse... */
			valid += 2;
			v = 1;
		}

		if (valid == 3) {		/* pulse qualified as 0 AND 1!, use closest match... */
			if ((abs(lp - b)) > (abs(sp - b)))
				v = 0;
			else
				v = 1;
		}
	}

	if (valid == 0) {			/* Error, pulse didnt qualify as either Bit0 or Bit1... */
		add_read_error(pos);
		v = -1;
	}

	if (valid == 4) {			/* Error, pulse is ON the threshold... */
		add_read_error(pos);
		v = -1;
	}

	return v;
}

/*
 * Generic "READ_BYTE" function, (can be used by most turbotape formats)
 * Reads and decodes 8 pulses from 'pos' in the TAP file.
 * parameters...
 * lp = ideal long pulse width.
 * sp = ideal short pulse width.
 * tp = threshold pulse width (can be NA if unknown)
 * return 0 or 1 on success else -1.
 * endi = endianess (use constants LSbF or MSbF).
 * Returns byte value on success, or -1 on failure.
 *
 * Note: Most formats can use this (and readttbit) for reading data, but some
 * (ie. cbmtape, pavloda, visiload,supertape etc) have custom readers held in their
 * own source files.
 */

int readttbyte(int pos, int lp, int sp, int tp, int endi)
{
	int i, v, b;
	unsigned char byte[10];

	/* check next 8 pulses are not inside a pause and *are* inside the TAP... */

	for(i = 0; i < 8; i++) {
		b = readttbit(pos + i, lp, sp, tp);
		if (b == -1)
			return -1;
		else
			byte[i] = b;
	}

	/* if we get this far, we have 8 valid bits... decode the byte... */

	if (endi == MSbF) {
		v = 0;
		for (i = 0; i < 8; i++) {
			if (byte[i] == 1)
				v += (128 >> i);
		}
	} else {
		v = 0;
		for (i = 0; i < 8; i++) {
			if (byte[i] == 1)
			v += (1 << i);
		}
	}

	return v;
}

/*---------------------------------------------------------------------------
 * Search for pilot/sync sequence at offset 'pos' in tap file...
 * 'fmt' should be the numeric ID (or constant) of a format described in ft[].
 *
 * Returns 0 if no pilot found.
 * Returns end position if pilot found (and a legal quantity of them).
 * Returns end position (negatived) if pilot found but too few/many.
 *
 * Note: End position = file offset of 1st NON pilot value, ie. a sync byte.
 */

int find_pilot(int pos, int fmt)
{
	int z, sp, lp, tp, en, pv, sv, pmin, pmax;

	if (pos < 20)
		return 0;

	sp = ft[fmt].sp;
	lp = ft[fmt].lp;
	tp = ft[fmt].tp;
	en = ft[fmt].en;
	pv = ft[fmt].pv;
	sv = ft[fmt].sv;
	pmin = ft[fmt].pmin;
	pmax = ft[fmt].pmax;

	if (pmax == NA)
		pmax = 200000;		/* set some crazy limit if pmax is NA (NA=0) */

	if ((pv == 0 || pv == 1) && (sv == 0 || sv == 1)) {	/* are the pilot/sync BIT values?... */
		if (readttbit(pos, lp, sp, tp) == pv) {		/* got pilot bit? */
			z = 0;
			while(readttbit(pos, lp, sp, tp) == pv && pos < tap.len) {	/* skip all pilot bits.. */
				z++;
				pos++;
			}

			if (z == 0)
				return 0;

			if (z < pmin || z > pmax)	/* too few/many pilots?, return position as negative. */
				return -pos;

			if (z >= pmin && z <= pmax)	/* enough pilots?, return position. */
				return pos;
		}
	} else {	/* Pilot/sync are BYTE values... */
		if (readttbyte(pos, lp, sp, tp, en) == pv) {	/* got pilot byte? */
			z = 0;
			while(readttbyte(pos, lp, sp, tp, en) == pv && pos < tap.len) {	/* skip all pilot bytes.. */
				z++;
				pos += 8;
			}

			if (z == 0)
				return 0;

			if (z < pmin || z > pmax)	/* too few/many pilots?, return position as negative. */
				return -pos;

			if (z >= pmin && z <= pmax)	/* enough pilots?, return position. */
				return pos;
		}
	}

	return 0;
}

/*
 * Load the named tap file into buffer (tap.tmem[])
 *
 * @param name	Name of the tap file to load
 *
 * @return 1 on success
 * @return 0 on error.
 */

int load_tap(char *name)
{
	FILE *fp;
	size_t flen;
	unsigned char *input_buffer, *output_buffer;

	fp = fopen(name, "rb");
	if (fp == NULL) {
		msgout("\nError: Couldn't open file in load_tap().");
		return 0;
	}

	/* First erase all stored data for the loaded 'tap', free buffers,
	   and reset database entries */
	unload_tap();

	/* Read file size */
	fseek(fp, 0, SEEK_END);
	flen = ftell(fp);
	rewind(fp);

	/* Allocate enough space to load the file into */
	input_buffer = (unsigned char*)malloc(flen);
	if (input_buffer == NULL) {
		msgout("\nError: malloc failed in load_tap().");
		fclose(fp);
		return 0;
	}

	/* Load data into buffer */
	fread(input_buffer, flen, 1, fp);
	fclose(fp);

	/* Check for DC2N format */
	if (strncmp(DC2N_ID_STRING, (char *)input_buffer, strlen(DC2N_ID_STRING)) == 0) {
		msgout("\nDC2N 16-bit TAP format detected. Converting to legacy TAP v1 (assuming 16-bit and 2MHz.)");

		output_buffer = (unsigned char*)malloc(flen);
		if (output_buffer == NULL) {
			msgout("\nError: malloc failed in load_tap().");
			free (input_buffer);
			return 0;
		}

		tap.len = convert_dc2n(input_buffer, output_buffer, flen);
		tap.tmem = output_buffer;
		free (input_buffer);
	} else {
		tap.tmem = input_buffer;

		/* Set the 'tap' structure file length subfield */
		tap.len = flen;
	}


	/* the following were uncommented in GUI app..
	 * these now appear in main()...
	 * tol=DEFTOL;        reset tolerance..
	 * debugmode=FALSE;      reset "debugging" mode.
	 */


	/* If desired, preserve loader table between TAPs... */
	if (preserveloadertable == FALSE)
		reset_loader_table();

	/* Makes sure the tap is fully scanned afterwards. */
	tap.changed = TRUE;

	/* Reset this so cbm parts decode for a new tap
	   (but NOT during same tap) */
	cbm_decoded = FALSE;

	/* Set the 'tap' structure file path and name subfields */
	strcpy(tap.path, name);
	getfilename(tap.name, name);
       
	/* Enable skew adapting in case the command line option was given.
	 * Re-enable it if more than one input file was selected for analysis.
	 */
	if (skewadapt)
		skewadapt_enabled = TRUE;

	return 1;		/* 1 = loaded ok */
}

/**
 * Perform a full analysis of the tap file
 *
 * Gather all available info from the tap. most data is stored in the 'tap' struct.
 *
 * Text output for pulse and file stats are written to str_pulses[] & str_files[] (global char arrays)
 * Note: this text output is created for the benefit of batchmode. (which doesnt call report()).
 *
 * Return 0 if file is not a valid TAP, else 1.
 */

int analyze(void)
{
	double per;

	if (tap.tmem == NULL)
		return 0;		/* no tap file loaded */

	tap.fsigcheck = check_signature();
	tap.fvercheck = check_version();
	tap.fsizcheck = check_size();

	if (tap.fsigcheck == 1 && tap.fvercheck == 1 && tap.fsizcheck == 1) {
		msgout("\nError: File is not a valid C64 TAP.");	/* all header checks failed */
		return 0;
	}

	tap.taptime = get_duration(20, tap.len);

	/* While cleaning, analyze() is called very often: the same 
	   information is appended to the 'info' buffer continuously.
	   This may lead to buffer overflow (Super Man, Visiload T1) so 
	   that we empty the buffer here for it's not used anyway */
	strcpy(info, "");	/* clear 'info' ready to receive new text */

	/* now call search_tap() to fill the file database (blk) */
	/* + call describe_blocks() to extract data and get checksum info. */

	note_errors = FALSE;
	search_tap();
	note_errors = TRUE;
	describe_blocks();
	note_errors = FALSE;

	/* Gather statistics... */

	tap.purity = get_pulse_stats();		/* note: saves unique pulse count */
	get_file_stats();

	tap.optimized_files = count_opt_files();
	tap.total_checksums_good = count_good_checksums();
	tap.detected = count_rpulses();
	tap.bootable = count_bootparts();

	/* compute % recognised... */

	per = ((double)tap.detected / ((double)tap.len - 20)) * 100;
	tap.detected_percent = floor(per);

	/* Compute & store quality checks... */

	if (tap.fsigcheck == 0 && tap.fvercheck == 0 && tap.fsizcheck == 0)
		tap.tst_hd = 0;
	else
		tap.tst_hd = 1;

	if (tap.detected == (tap.len - 20))
		tap.tst_rc = 0;
	else
		tap.tst_rc = 1;

	if (tap.total_checksums_good == tap.total_checksums)
		tap.tst_cs = 0;
	else
		tap.tst_cs = 1;

	if (tap.total_read_errors == 0)
		tap.tst_rd = 0;
	else
		tap.tst_rd = 1;

	if (tap.total_data_files - tap.optimized_files == 0)
		tap.tst_op = 0;
	else
		tap.tst_op = 1;

	tap.crc = compute_overall_crc();

	return 1;
}

/*
 * Save a report for this TAP file.
 * Note: Call 'analyze()' before calling this!.
 */

void report(void)
{
	int i;
	FILE *fp;
	char *rbuf;

	rbuf = (char*)malloc(1000000);
	if (rbuf == NULL) {
		msgout("\nError: malloc failed in report().");
		exit(1);
	}

	chdir(exedir);

	fp = fopen(temptcreportname, "r");	/* delete any existing temp file... */
	if (fp != NULL) {
		fclose(fp);
		unlink (temptcreportname);
	}

	fp = fopen(tcreportname, "r");		/* delete any existing report file... */
	if (fp != NULL) {
		fclose(fp);
		unlink (tcreportname);
	}     

	fp = fopen(temptcreportname, "w+t");	/* create new report file... */

	if (fp != NULL) {

		/* include results and general info... */

		print_results(rbuf);
		fprintf(fp, rbuf);
		fprintf(fp, "\n\n\n\n\n");

		/* include file stats... */

		print_file_stats(rbuf);
		fprintf(fp, rbuf);
		fprintf(fp, "\n\n\n\n\n");

		/* include database in the file (partially interpreted)... */

		print_database(rbuf);
		fprintf(fp, rbuf);
		fprintf(fp, "\n\n\n\n\n");

		/* include pulse stats in the file... */

		print_pulse_stats(rbuf);
		fprintf(fp, rbuf);
		fprintf(fp, "\n\n\n\n\n");

		/* include 'read errors' report in the file... */

		if (tap.total_read_errors != 0) {
			fprintf(fp, "\n * Read error locations (Max %d)", NUM_READ_ERRORS);
			fprintf(fp, "\n");
			for (i = 0; read_errors[i] != 0; i++)
				fprintf(fp, "\n0x%04X", read_errors[i]);
			fprintf(fp, "\n\n\n\n\n");
		}
		fclose(fp);

		//sprintf(lin, OSAPI_RENAME_FILE" %s %s", temptcreportname, tcreportname);
		//system(lin);
		rename (temptcreportname, tcreportname);
	} else
		msgout("\nError: failed to create report file.");
       
	/* show results and general info onscreen... */

	print_results(rbuf);
   
	if (!batchmode)
		fprintf(stdout, rbuf);

	free(rbuf);

	if (doprg == TRUE) {
		make_prgs();
		save_prgs();
	}
}

/*
 * Calls upon functions found in clean.c to optimize and
 * correct the TAP.
 */

void clean(void)
{
	int x;

	if (tap.tmem == NULL) {
		msgout("\nError: No TAP file loaded!.");
		return;
	}

	if(debug) {
		msgout("\nError: Optimization is disabled whilst in debugging mode.");
		return;
	}

	quiet = 1;		/* no talking between search routines and worklog */

	convert_to_v0();	/* unpack pauses to V0 format if not already */
	clip_ends();		/* clip leading and trailing pauses */
	unify_pauses();		/* connect/rebuild any consecutive pauses */
	clean_files();		/* force perfect pulsewidths on known blocks */
	convert_to_v1();	/* repack pauses (v1 format) */

	fill_cbm_tone();	/* presently fills any gap of around 80 pulses   */
				/* (following a CBM block) with ft[CBM_HEAD].sp's. */

	/* this loop repairs pilot bytes and small gaps surrounding pauses, if
	 * any pauses are inserted by 'insert_pauses()' then new gaps and pilots
	 * may be identified in which case we repeat the loop until they are all
	 * dealt with.
	 */
	do {
		fix_pilots();		/* replace broken pilots with new ones. */
		fix_prepausegaps();	/* fix all pre pause "spike runs" of 1 2 or 3. */
		fix_postpausegaps();	/* fix all post pause "spike runs" of 1 2 or 3. */
		x = insert_pauses();	/* insert pauses between blocks that need one. */
	} while(x);

	standardize_pauses();		/* standardize CBM HEAD -> CBM PROG pauses. */
	fix_boot_pilot();		/* add new $6A00 pulse pilot on a CBM boot header. */
	cut_postdata_gaps();		/* cuts post-data gaps <20 pulses. */
	cut_leading_gap();              /* cuts gap <20 pulses found at beginning of tape. */

	if (noaddpause == FALSE)
		add_trailpause();	/* add a 5 second trailing pause   */

	fix_bleep_pilots();		/* correct any corrupted bleepload pilots */

	msgout("\n");
	msgout("\nCleaning finished.");
	quiet = 0;			/* allow talking again. */
}

/*
 * Check whether the offset 'x' is accounted for in the database (by a data file
 * or a pause, not a gap), return 1 if it is, else 0.
 */

int is_accounted(int x)
{
	int i;

	for (i = 0; blk[i]->lt != 0; i++) {
		if (blk[i]->lt != GAP) {
			if ((x >= blk[i]->p1) && (x <= blk[i]->p4))
				return 1;
		}
	}

	return 0;
}

/*
 * Checks whether location 'p' is inside a pause. (harder than it sounds!)
 * Return 1 if it is, 0 if not.
 * Returns -1 if index is out-of-bounds
*/

int is_pause_param(int p)
{
	int i, z, pos;

	if (p < 20 || p > tap.len - 1)	/* p is out of bounds  */
		return -1;

	if (tap.tmem[p] == 0)		/* p is pointing at a zero! */
		return 1;

	if (tap.version == 0)		/* previous 'if' would have dealt with v0. the rest is v1 only */
		return 0;
   
	if (p < 24) {			/* test very beginning of TAP file, ensures no rewind into header! */
		if (tap.tmem[20] == 0)
			return 1;
		else
			return 0;
	}

	pos = p - 4;			/* pos will be at least 20 */

	do {				/* find first 4 pulses containing no zeroes (behind p)... */
		z = 0;
		for (i = 0; i < 4; i++) {
			if(tap.tmem[pos + i] == 0)
				z++;
		}
		if (z != 0)
			pos--;
	} while(z != 0 && pos > 19);
 

	if (z == 0) {			/* if TRUE, we found the first 4 containing no zeroes (behind p) */
		pos += 4;		/* pos now points to first v1 pause (a zero)  */

		/* ie.          xxxxxxxxxxx 0xx0 00xx 0x0x x 0x0x 00xx */
		/*			    ^=pos          ^ = p */
 
		for (i = pos; i < tap.len - 4 ; i++) {
			if (tap.tmem[i] == 0)	/* skip over v1 pauses */
				i += 3;
			else {
				if (i == p)
					return 0;	/* p is not in a pause */

				if (i > p)
					return 1;	/* p is in a pause */
			}
		}
	} else { /* luigi: just start at the beginning then */
		for (i = 20; i < tap.len - 4 ; i++) {
			if (tap.tmem[i] == 0)	/* skip over v1 pauses */
				i += 3;
			else {
				if (i == p)
					return 0;	/* p is not in a pause */

				if (i > p)
					return 1;	/* p is in a pause */
			}
		}
	}
	return 0; /* p is the pos of one of the last 4 pulses in the TAP file */
}

/*
 * search blk[] array for instance number 'num' of block type 'lt' and calls
 * 'xxx_describe_block' for that file (which decodes it).
 * this is for use by scanners which need to get access to data held in other files
 * ahead of describe() time.
 * returns the block number in blk[] of the matching (and now decoded) file.
 * on failure returns -1;
 *
 * NOTE : currently only implemented for certain file types.
 */

int find_decode_block(int lt, int num)
{
	int i, j;

	for (i = 0,j = 0; i < BLKMAX; i++) {
		if (blk[i]->lt == lt) {
			j++;
			if (j == num) {		/* right filetype and right instance number? */
				if (lt == CBM_DATA || lt == CBM_HEAD) {
					cbm_describe(i);
					return i;
				}

				if (lt == CYBER_F1) {
					cyberload_f1_describe(i);
					return i;
				}

				if (lt == CYBER_F2) {
					cyberload_f2_describe(i);
					return i;
				}
			}
		}
	}

	return -1;
}

/*
 * Add an entry to the 'read_errors[NUM_READ_ERRORS]' array...
 */

int add_read_error(int addr)
{
	int i;

	if (!note_errors)
		return -1;

	for (i = 0; i < NUM_READ_ERRORS; i++) {				/* reject duplicates.. */
		if (read_errors[i] == addr)
			return -1;
	}

	for (i = 0; read_errors[i] != 0 && i < NUM_READ_ERRORS; i++);	/* find 1st free slot.. */

	if (i < NUM_READ_ERRORS) {
		read_errors[i] = addr;
		return 0;
	}

	return -1;		/* -1 = error table is full */
}

/*
 * Displays a message.
 * I made this to quickly convert the method of text output from the windows
 * sources. (ie. popup message windows etc).
 */

void msgout(char *str)
{
	printf("%s", str);
}

/*
 * Search integer array 'buf' for occurrence of sequence 'seq'.
 * On success return offset of matching sequence.
 * On failure return -1.
 * Note : value XX (-1) may be used in 'seq' as a wildcard.
 */

int find_seq(int *buf, int bufsz, int *seq, int seqsz)
{
	int i, j, match;

	if (seqsz > bufsz)			/* buf must be larger or equal to seq */
		return -1;

	for (i = 0; i < bufsz - seqsz; i++) {
		if (buf[i] == seq[0]) {		/* match first number. */
			match = 0;
			for (j = 0; j < seqsz && (i + j) < bufsz; j++) {
				if (buf[i + j] == seq[j] || seq[j] == -1)
					match++;
			}
			if (match == seqsz)	/* whole sequence found?  */
				return i;
		}
	}

	return -1;
}

/*
 * Isolate the filename part of a full path+filename and store it in buffer *dest.
 */

void getfilename(char *dest, char *fullpath)
{
	int i, j, k;

	i = strlen(fullpath);
	for (j = i; j > 0 && fullpath[j] != SLASH; j--);

	/* rewind j to 0 or first slash.. */

	if (fullpath[j] == SLASH)
		j++;		/* skip over the slash */
	for (k = 0; j < i; j++)
		dest[k++] = fullpath[j];
	dest[k] = 0;

	return;
}

/*
 * Change file extension if any is already in filename
 */
char* change_file_extention (char *filename, char *new_extension, int buffer_length)
{
        int i, len;

	len = strlen(filename);
	for (i = len - 1; i > 0 && filename[i] != '.'; i--)
		;

	if (filename[i] == '.')
	{
		int j;

	        i++;
	        len = strlen(new_extension);
	        for (j = 0; j < len && i+j < buffer_length - 1; j++)
	                filename[i+j] = new_extension[j];
		filename[i+j] = '\0';
	}

	return filename;
}

/*
 * convert PetASCII string to ASCII text string.
 * user provides destination storage string 'dest'.
 * function returns a pointer to dest so the function may be called inline.
 */

char* pet2text(char *dest, char *src)
{
	int i, lwr;
	char ts[500];
	unsigned char ch;

	lwr = 0;	/* lowercase off. */

	/* process file name... */

	strcpy(dest, "");
	for (i = 0; src[i] != 0; i++) {
		ch = (unsigned char)src[i];

		/* process CHR$ 'SAME AS' codes... */

		if (ch == 255)
			ch = 126;
		if (ch > 223 && ch < 255)	/* produces 160-190 */
			ch -= 64;
		if (ch > 191 && ch < 224)	/* produces 96-127 */
			ch -= 96;

		if (ch == 14)			/* switch to lowercase.. */
			lwr = 1;
		if (ch == 142)			/* switch to uppercase.. */
			lwr = 0;

		if (ch > 31 && ch < 128) {	/* print printable character... */
			if (lwr) {		/* lowercase?, do some conversion... */
				if (ch > 64 && ch < 91)
					ch += 32;
				else if (ch > 96 && ch < 123)     
					ch -= 32;
			}

			sprintf(ts, "%c", ch);
			strcat(dest, ts);
		}
	}

	return dest;
}

/*
 * Trims trailing spaces from a string.
 */

void trim_string(char *str)
{
	int i, len;
   
	len = strlen(str);
	if (len > 0) {
		for (i = len - 1; str[i] == 32 && i > 0; i--)	/* nullify trailing spaces.  */
		str[i] = 0;
	}
}

/*
 * Pads the string 'str' with spaces so the resulting string is 'wid' chars long. 
 */

void padstring(char *str, int wid)
{
	int i, len;

	len = strlen(str);
	if (len < wid) {
		for (i = len; i < wid; i++)
			str[i] = 32;
		str[i] = 0;
	}
}

/*
 * Converts an integer number of seconds to a time string of format HH:MM:SS.
 */

void time2str(int secs, char *buf)
{
	int h, m, s;

	h = secs / 3600;
	m = (secs- (h * 3600)) / 60;
	s = secs - (h * 3600) - (m * 60);
	sprintf(buf, "%02d:%02d:%02d", h, m, s);
}

/*
 * Remove all/any existing work files. 
 */

void deleteworkfiles(void)
{
	FILE *fp;
      
	/* delete existing work files... */
	/* note: the fopen tests avoid getting any console error output. */   
 
	fp = fopen(temptcreportname, "r");    
	if (fp != NULL) {
		fclose(fp);
		unlink (temptcreportname);
	}

	fp = fopen(tcreportname, "r");     
	if (fp != NULL) {
		fclose(fp);
		unlink (tcreportname);
	}

	fp = fopen(temptcbatchreportname, "r");      
	if (fp != NULL) {
		fclose(fp);
		unlink (temptcbatchreportname);
	}

	fp = fopen(tcbatchreportname, "r");    
	if (fp != NULL) {
		fclose(fp);
		unlink (tcbatchreportname);
	}

	fp = fopen(tcinfoname, "r");   
	if (fp != NULL) {
		fclose(fp);
		unlink (tcinfoname);
	}  
}  

