/**
 *	@file 	persistence.h
 *	@brief	A module to reuse discovered loader parameters in subsequent scans.
 *
 *	This module persists discovered loader parameters in subsequent 
 *  invokations of the application.
 */

#ifndef TC_PERSISTENCE_H
#define TC_PERSISTENCE_H

enum {
	PERS_OK = 0,
	PERS_LOCK_TIMEOUT,
	PERS_OPEN_ERROR,
	PERS_UNSUPPORTED_VERSION,
	PERS_IO_ERROR
};

int persistence_load_loader_parameters (void);
int persistence_save_loader_parameters (void);

#endif
