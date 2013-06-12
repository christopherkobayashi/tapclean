#ifndef TC_PERSISTENCE_H
#define TC_PERSISTENCE_H

enum {
	PERS_OK = 0,
	PERS_LOCK_TIMEOUT,
	PERS_OPEN_ERROR,
	PERS_UNSUPPORTED_VERSION,
	PERS_IO_ERROR
};

int load_persistent_data (void);
int save_persistent_data (void);

#endif
