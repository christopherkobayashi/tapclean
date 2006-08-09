/**
 *	@file 	database.h
 *	@brief	A tap database for recognized files.
 *
 *	Details here.
 */

#ifndef __BLKDATABASE_H__
#define __BLKDATABASE_H__

#define BLKMAX	2000	/*!< maximum number of blocks allowed in database */
#define DBERR	-1	/*!< return value from "addblockdef" when database 
			     entry failed. */
#define DBFULL	-2	/*!< return value from "addblockdef" when database 
			     is full. */
#define HASNOTCHECKSUM	-2

/**
 *	Struct 'blk_t'
 *
 *	This is the basic unit of the tap database, each entity found in
 *	a tap file will have one of these, see array 'blk[]'...  
 */

struct blk_t
{
	int lt;			/*!< loader type (see loadername enum of 
	    			     constants above) */
	int p1;			/*!< first pulse */
	int p2;			/*!< first data pulse */
	int p3;			/*!< last data pulse */
	int p4;			/*!< last pulse */
	int xi;			/*!< extra info */

	int cs;			/* c64 ram start pos */
	int ce;			/* c64 ram end pos */
	int cx;			/* c64 ram len */
	unsigned char *dd;	/* pointer to decoded data block */
	int crc;		/* crc32 of the decoded data file */
	int rd_err;		/* number of read errors in block */
	int cs_exp;		/* expected checksum value (if applicable) */
	int cs_act;		/* actual checksum value (if applicable) */
	int pilot_len;		/* length of pilot tone (in bytes or pulses) */
	int trail_len;		/* length of trail tone (in bytes or pulses) */
	char *fn;		/* pointer to file name (if applicable) */
	int ok;			/* file ok indicator, 1=ok. */ 
};

extern struct blk_t *blk[BLKMAX];

/**
 *	Prototypes
 */

int create_database(void);
void reset_database(void);
int addblockdef(int, int, int, int, int, int);
void sort_blocks(void);
void scan_gaps(void);
int count_rpulses(void);
int count_good_checksums(void);
int compute_overall_crc(void);
void destroy_database(void);

#endif
