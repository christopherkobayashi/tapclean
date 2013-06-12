#include <stdio.h>
#include <string.h>
#include <unistd.h>
#ifdef WIN32
#include <direct.h>
#endif

#include "mydefs.h"
#include "persistence.h"

static const char *persistentstore = "persistence.ini";
static const char *persistencetmp = "persistence.tmp";

int load_persistent_data (void)
{
	FILE *pFile;
	char  readbuffer[64];
	int   retries, i;

	chdir(exedir);

	for (retries = 0;;) {
		pFile = fopen (persistencetmp, "r");
		if (pFile == NULL)
			break;
		fclose (pFile);

		retries++;
		if (retries >= 3)
			return 1;

		usleep(1000);
	}

	pFile = fopen (persistentstore, "r");
	if (pFile == NULL)
		return 2;

	while (fgets (readbuffer, 64, pFile) != NULL) {
		char loader[32];
		int  en, tp, sp, mp, lp, pv, sv;

		if (sscanf (readbuffer, 
				"%s %d %d %d %d %d %d %d",
				loader,
				&en,
				&tp,
				&sp,
				&mp,
				&lp,
				&pv,
				&sv) == 8) {

			for (i = 0; i < strlen(loader); i++)
				if (loader[i] == '~')
					loader[i] = ' ';

			for (i = CBM_HEAD; i < sizeof(ft)/sizeof(ft[0]) && ft[i].name[0]; i++)
				if (strncmp (loader, ft[i].name, strlen(ft[i].name)) == 0) {
					ft[i].en= en;
					ft[i].tp= tp;
					ft[i].sp= sp;
					ft[i].mp= mp;
					ft[i].lp= lp;
					ft[i].pv= pv;
					ft[i].sv= sv;

					break;
				}
		}
	}

	fclose (pFile);

	return 0;
}

int save_persistent_data (void)
{
	FILE *pFile;
	int   retries, i;

	chdir(exedir);

	for (retries = 0;;) {
		pFile = fopen (persistencetmp, "r");
		if (pFile == NULL)
			break;
		fclose (pFile);

		retries++;
		if (retries >= 3)
			return 1;

		usleep(1000);
	}

	pFile = fopen (persistencetmp, "w+");
	if (pFile == NULL)
		return 2;

	fputs ("# TAPClean persistence file - do NOT edit\n", pFile);
	fputs ("[persistence]\n", pFile);
	fputs ("version = 1\n", pFile);
	fputs ("[loader_values]\n", pFile);

	for (i = CBM_HEAD; i < sizeof(ft)/sizeof(ft[0]) && ft[i].name[0]; i++) {
		char writebuffer[64];
		int  j;

		sprintf (writebuffer,
				"%s %d %d %d %d %d %d %d\n",
				ft[i].name,
				ft[i].en,
				ft[i].tp,
				ft[i].sp,
				ft[i].mp,
				ft[i].lp,
				ft[i].pv,
				ft[i].sv);

		for (j = 0; j < strlen(ft[i].name); j++)
			if (writebuffer[j] == ' ')
				writebuffer[j] = '~';

		fputs (writebuffer, pFile);
	}

	fclose (pFile);

	unlink (persistentstore);

	rename (persistencetmp, persistentstore);

	return 0;
}
