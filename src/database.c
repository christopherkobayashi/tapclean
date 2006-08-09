/**
 *	@file 	database.c
 *	@brief	A tap database for recognized files.
 *
 *	Details here.
 */

#include "mydefs.h"
#include "database.h"
#include "main.h"

struct blk_t *blk[BLKMAX];	/*!< Database of all found entities. */

/**
 *	Allocate ram to file database and initialize array pointers.
 *
 *	@param void
 *
 *	@return TRUE on success
 *	@return FALSE on memory allocation failure
 */
 
int create_database(void)
{
	int i;

	for (i = 0; i < BLKMAX; i++) {
		blk[i] = (struct blk_t*)malloc(sizeof(struct blk_t));
		if (blk[i] == NULL) {
			printf("\nError: malloc failure whilst creating file database.");
			destroy_database(); /* Free any already allocated resource */
			return FALSE;
		}

		blk[i]->dd = NULL;
		blk[i]->fn = NULL;
	}
	
	return TRUE;
}

/**
 *	Clear database 
 *	
 *	@param void
 *
 *	@return none
 */
 
void reset_database(void)
{
	int i;

	for (i = 0 ; i < BLKMAX; i++) {		/*!< clear database... */
		blk[i]->lt = 0;
		blk[i]->p1 = 0;
		blk[i]->p2 = 0;
		blk[i]->p3 = 0;
		blk[i]->p4 = 0;
		blk[i]->xi = 0;
		blk[i]->cs = 0;
		blk[i]->ce = 0;
		blk[i]->cx = 0;
		blk[i]->crc = 0;
		blk[i]->rd_err = 0;
		blk[i]->cs_exp = HASNOTCHECKSUM;
		blk[i]->cs_act = HASNOTCHECKSUM;
		blk[i]->pilot_len = 0;
		blk[i]->trail_len = 0;
		blk[i]->ok = 0;

		if (blk[i]->dd != NULL) {
			free(blk[i]->dd);
			blk[i]->dd = NULL;
		}
		
		if(blk[i]->fn != NULL) {
			free(blk[i]->fn);
			blk[i]->fn = NULL;
		}
	}
}

/**
 *	Add a block definition (file details) to the database (blk)
 *
 *	Only sof & eof must be assigned (legal) values for the block,
 *	the others can be 0.
 *
 *	@param lt loader id
 *	@param sof start of block offset
 *	@param sod start of data offset
 *	@param eod end of data offset
 *	@param eof end of block offset
 *	@param xi extra info offset
 *	
 *	@return Slot number the block went to
 *	@return DBERR on invalid block definition
 *	@return DBFULL on database full
 */
 
int addblockdef(int lt, int sof, int sod, int eod, int eof, int xi)
{
	int i, slot, e1, e2;

	if (debug == FALSE) {

		/* check that the block does not conflict with any existing blocks... */

		for (i = 0; blk[i]->lt != 0; i++) {
			e1 = blk[i]->p1;	/* get existing block start pos  */
			e2 = blk[i]->p4;	/* get existing block end pos   */

			if (!((sof < e1 && eof < e1) || (sof > e2 && eof > e2)))
				return DBERR;
		}
	}

	if ((sof > 19 && eof < tap.len) && (eof >= sof)) {

		/* find the first free slot (containing 0 in 'lt' field)... */

		/* note: slot blk[BLKMAX-1] is reserved for the list terminator. */
		/* the last usable slot is therefore BLKMAX-2. */

		for (i = 0; blk[i]->lt != 0; i++);
			slot = i;

		if (slot == BLKMAX-1) {	/* only clear slot is the last one? (the terminator) */
			if (dbase_is_full == FALSE) {	/* we only need give the error once */
				if (!batchmode)		/* dont bother with the warning in batch mode.. */
					msgout("\n\nWarning: FT's database is full...\nthe report will not be complete.\nTry optimizing.\n\n");
				dbase_is_full = TRUE;
			}
			return DBFULL;
		} else {

			/* put the block in the last available slot... */

			blk[slot]->lt = lt;
			blk[slot]->p1 = sof;
			blk[slot]->p2 = sod;
			blk[slot]->p3 = eod;
			blk[slot]->p4 = eof;
			blk[slot]->xi = xi;

			/* just clear out the remaining fields... */

			blk[slot]->cs = 0;
			blk[slot]->ce = 0;
			blk[slot]->cx = 0;
			blk[slot]->dd = NULL;
			blk[slot]->crc = 0;
			blk[slot]->rd_err = 0;
			blk[slot]->cs_exp = HASNOTCHECKSUM;
			blk[slot]->cs_act = HASNOTCHECKSUM;
			blk[slot]->pilot_len = 0;
			blk[slot]->trail_len = 0;
			blk[slot]->fn = NULL;
			blk[slot]->ok = 0;
		}
	} else
		return DBERR;

	return slot;	/* ok, entry added successfully.   */
}

/**
 *	Sort the database by p1 (file start position, sof) values.
 *	
 *	@param void
 *
 *	@return none
 */

void sort_blocks(void)
{
	int i, swaps,size;
	struct blk_t *tmp;

	for (i = 0; blk[i]->lt != 0 && i < BLKMAX; i++);

	size = i;	/* store current size of database */
   
	do {
		swaps = 0;
		for (i = 0; i < size - 1; i++) {

			/* examine file sof's (p1's), swap if necessary... */

			if ((blk[i]->p1) > (blk[i + 1]->p1)) {
				tmp = blk[i];
				blk[i] = blk[i + 1];
				blk[i + 1] = tmp;
				swaps++;  
			}
		}
	} while(swaps != 0);	/* repeat til no swaps occur.  */
}

/*
 * Searches the file database for gaps. adds a definition for any found.
 * Note: Must be called ONLY after sorting the database!.
 * Note : The database MUST be re-sorted after a GAP is added!.
 */

void scan_gaps(void)
{
	int i, p1, p2, sz;

	p1 = 20;			/* choose start of TAP and 1st blocks first pulse  */
	p2 = blk[0]->p1;

	if (p1 < p2) {
		sz = p2 - p1;
		addblockdef(GAP, p1, 0, 0, p2 - 1, sz);
		sort_blocks();
	}

	/* double dragon sticks in this loop */

	for (i = 0; blk[i]->lt != 0 && blk[i + 1]->lt != 0; i++) {
		p1 = blk[i]->p4;		/* get end of this block */
		p2 = blk[i + 1]->p1;		/* and start of next  */
		if (p1 < (p2 - 1)) {
			sz = (p2 - 1) - p1;
			if (sz > 0) {
				addblockdef(GAP, p1 + 1, 0, 0, p2 - 1, sz);
				sort_blocks();
			}
		}
	}
   
	p1 = blk[i]->p4;		/* choose last blocks last pulse and End of TAP */
	p2 = tap.len - 1;
	if (p1 < p2) {
		sz = p2 - p1;
		addblockdef(GAP, p1 + 1, 0, 0, p2, sz);
		sort_blocks();
	}
}

/*
 * Return the number of pulses accounted for in total across all known files.
 */

int count_rpulses(void)
{
	int i, tot;

	/* add up number of pulses accounted for... */

	/* for each block entry in blk */

	for (i = 0, tot = 0; blk[i]->lt != 0; i++) {
		if (blk[i]->lt != GAP) {

			/* start and end addresses both present?  */

			if (blk[i]->p1 != 0 && blk[i]->p4 != 0)
				tot += (blk[i]->p4 - blk[i]->p1) + 1;
		}
	}

	return tot;
}

/*
 * Returns the quantity of 'has checksum and its OK' files in the database.
 */

int count_good_checksums(void)
{
	int i, c;

	for (i = 0,c = 0; blk[i]->lt != 0; i++) {
		if (blk[i]->cs_exp != HASNOTCHECKSUM) {
			if (blk[i]->cs_exp == blk[i]->cs_act)
				c++;
		}
	}

	return c;
}

/*
 * Add together all data file CRC32's
 */

int compute_overall_crc(void)
{
	int i, tot = 0;

	for (i = 0; blk[i]->lt != 0; i++)
		tot += blk[i]->crc;

	return tot;
}

/**
 *	Deallocate file database from RAM
 *
 *	A check for non-NULL is done, in case we are freeing resources
 *	after a malloc failure in create_database() (clean job).
 *
 *	@param void
 *
 *	@return none
 */
 
void destroy_database(void)
{
	int i;

	for (i = 0; i < BLKMAX && blk[i] != NULL; i++)
		free(blk[i]);
}
