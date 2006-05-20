/**
 *	@file 	database.c
 *	@brief	A tap database for recognized files.
 *
 *	Details here.
 */

#include "mydefs.h"
#include "database.h"

struct blk_t *blk[BLKMAX];	/*!< Database of all found entities. */

/**
 *	Allocate ram to file database and initialize array pointers.
 *
 *	@param void
 *
 *	@return 0 on success
 *	@return 1 on memory allocation failure
 */
 
int create_database(void)
{
	int i;

	for (i = 0; i < BLKMAX; i++) {
		blk[i] = (struct blk_t*)malloc(sizeof(struct blk_t));
		if (blk[i] == NULL) {
			printf("\nError: malloc failure whilst creating file database.");
			return 1;
		}

		blk[i]->dd = NULL;
		blk[i]->fn = NULL;
	}
	
	return 0;
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
		blk[i]->cs_exp = -2;
		blk[i]->cs_act = -2;
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
			blk[slot]->cs_exp = -2;
			blk[slot]->cs_act = -2;
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

/**
 *	Deallocate file database from RAM
 *	
 *	@param void
 *
 *	@return none
 */
 
void destroy_database(void)
{
	int i;

	for (i = 0; i < BLKMAX; i++)
		free(blk[i]);
}
