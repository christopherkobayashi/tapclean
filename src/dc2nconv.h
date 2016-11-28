/**
 *	@file 	dc2nconv.h
 *	@brief	Conversion routines to handle DC2N DMP files.
 *
 *	The following code converts DC2N DMP files to TAP files. It assumes
 *	tapes for a C64 (PAL) were sampled at 20MHz.
 */

#ifndef __DC2NCONV_H__
#define __DC2NCONV_H__

int convert_dc2n(unsigned char *, unsigned char *, int);

#endif
