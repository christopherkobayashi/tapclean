/**
 *	@file 	dc2nconv.c
 *	@brief	Conversion routines to handle DC2N DMP files.
 */

#include "mydefs.h"
#include "dc2nconv.h"
#include "main.h"

#define C16_TAPE_RAW_SUPPORT /* Use Markus' extension as with MTAP */

// TODO: define a structure and some enums, rather than so many defs
#define TAP_FORMAT_VERSION_OFFSET	0x0C

#ifdef C16_TAPE_RAW_SUPPORT
#define TAP_FORMAT_PLATFORM_OFFSET	0x0D
#define TAP_FORMAT_VIDEO_STD_OFFSET	0x0E
#define TAP_FORMAT_PLATFORM_C64		0x00
#define TAP_FORMAT_PLATFORM_VIC20	0x01
#define TAP_FORMAT_PLATFORM_C16		0x02
#define TAP_FORMAT_VIDEO_PAL		0x00
#define TAP_FORMAT_VIDEO_NTSC		0x01
#define DC2N_FORMAT_PLATFORM_OFFSET	0x0D
#define DC2N_FORMAT_VIDEO_STD_OFFSET	0x0E
#else
#define TAP_FORMAT_EXP1_OFFSET		0x0D
#define TAP_FORMAT_EXP2_OFFSET		0x0E
#endif

#define TAP_FORMAT_EXP3_OFFSET		0x0F

/*
 * Write a long pulse to the buffer
 *
 * @param output_buffer	Buffer where the long pulse size is written
 * @param lp		Long pulse size in machine clock cycles/8
 *
 * @return Amount of bytes written to output buffer
 */

static size_t write_long_pulse(unsigned char *output_buffer, unsigned long lp)
{
	unsigned int zerot;
	int wbytes = 0;

	lp <<= 3;

	while (lp != 0) {
		if (lp >= 0x1000000)
			zerot = 0xffffff;	/* zerot =  maximum */
		else
			zerot = lp;
		lp -= zerot;

		output_buffer[0+wbytes] = 0x00;
		output_buffer[1+wbytes] = (unsigned char) (zerot & 0xFF);
		output_buffer[2+wbytes] = (unsigned char) ((zerot>>8) & 0xFF);
		output_buffer[3+wbytes] = (unsigned char) ((zerot>>16) & 0xFF);
		
		wbytes += 4;
	}

	return wbytes;
}

/*
 * Conversion routine to downsample from 2 MHz to TAP,
 * assuming PAL video standard and Commodore 64
 *
 * @param utime		DMP data sample
 *
 * @return Converted sample
 */
unsigned long downsample_c64_pal (unsigned long utime)
{
	// Exact PAL freq, 985248 Hz
	return (utime * 30789UL + 31250UL) / 62500UL; // PAL
}

#ifdef C16_TAPE_RAW_SUPPORT
/*
 * Conversion routine to downsample from 2 MHz to TAP
 * assuming NTSC video standard and Commodore 64
 *
 * @param utime		DMP data sample
 *
 * @return Converted sample
 */
unsigned long downsample_c64_ntsc (unsigned long utime)
{
	// NTSC freq approximated to 1022720 Hz
	return (utime * 15980UL + 15625UL) / 31250UL; // NTSC
}

/*
 * Conversion routine to downsample from 2 MHz to TAP,
 * assuming PAL video standard and VIC
 *
 * @param utime		DMP data sample
 *
 * @return Converted sample
 */
unsigned long downsample_vic_pal (unsigned long utime)
{
	// PAL freq approximated to 1108416 Hz
	return (utime * 17319UL + 15625UL) / 31250UL; // PAL
}

/*
 * Conversion routine to downsample from 2 MHz to TAP
 * assuming NTSC video standard and VIC
 *
 * @param utime		DMP data sample
 *
 * @return Converted sample
 */
unsigned long downsample_vic_ntsc (unsigned long utime)
{
	// NTSC freq approximated to 1022720 Hz
	return (utime * 15980UL + 15625UL) / 31250UL; // NTSC
}

/*
 * Conversion routine to downsample from 2 MHz to TAP,
 * assuming PAL video standard and Commodore 16
 *
 * @param utime		DMP data sample
 *
 * @return Converted sample
 */
unsigned long downsample_c16_pal (unsigned long utime)
{
	// PAL freq approximated to 886720 Hz
	return (utime * 27710UL + 31250UL) / 62500UL; // PAL
}

/*
 * Conversion routine to downsample from 2 MHz to TAP
 * assuming NTSC video standard and Commodore 16
 *
 * @param utime		DMP data sample
 *
 * @return Converted sample
 */
unsigned long downsample_c16_ntsc (unsigned long utime)
{
	// NTSC freq approximated to 894880 Hz
	return (utime * 27965UL + 31250UL) / 62500UL; // NTSC
}
#endif

/*
 * Convert DC2N format to TAP v1
 *
 * @param input_buffer	Buffer containing the DC2N dmp file
 * @param output_buffer	Buffer where the converted TAP is written
 * @param flen		Size of the DC2N DMP file
 *
 * @return Amount of useful bytes in the output buffer
 */

size_t convert_dc2n(unsigned char *input_buffer, unsigned char *output_buffer, size_t flen)
{
	size_t olen;
	size_t i;
	unsigned long utime, clockcycles, longpulse;
	unsigned long pulse;
#ifdef C16_TAPE_RAW_SUPPORT
	unsigned char platform, video_standard;
#endif

	unsigned long (*downsample)(unsigned long);

#ifdef C16_TAPE_RAW_SUPPORT
	strncpy((char *)output_buffer, TAP_ID_STRING, strlen(TAP_ID_STRING));
#endif

	output_buffer[TAP_FORMAT_VERSION_OFFSET] = 0x01;	/* convert to TAP v1 */

#ifdef C16_TAPE_RAW_SUPPORT
	platform = input_buffer[DC2N_FORMAT_PLATFORM_OFFSET];
	video_standard = input_buffer[TAP_FORMAT_VIDEO_STD_OFFSET];
#else
	output_buffer[TAP_FORMAT_EXP1_OFFSET] = 0x00;	/* initialize exp bytes */
	output_buffer[TAP_FORMAT_EXP2_OFFSET] = 0x00;
#endif
	output_buffer[TAP_FORMAT_EXP3_OFFSET] = 0x00;

	olen = TAP_HEADER_SIZE;
	longpulse = 0;

#ifdef C16_TAPE_RAW_SUPPORT
	switch (platform)
	{
		case TAP_FORMAT_PLATFORM_C64:
			if (video_standard == TAP_FORMAT_VIDEO_PAL)
			        downsample = downsample_c64_pal;
			else
			        downsample = downsample_c64_ntsc;
			break;
		case TAP_FORMAT_PLATFORM_VIC20:
			if (video_standard == TAP_FORMAT_VIDEO_PAL)
			        downsample = downsample_vic_pal;
			else
			        downsample = downsample_vic_ntsc;
			break;
		case TAP_FORMAT_PLATFORM_C16:
			if (video_standard == TAP_FORMAT_VIDEO_PAL)
			        downsample = downsample_c16_pal;
			else
			        downsample = downsample_c16_ntsc;
			break;
	}
	output_buffer[TAP_FORMAT_PLATFORM_OFFSET] = platform;
	output_buffer[TAP_FORMAT_VIDEO_STD_OFFSET] = video_standard;

#else /* C64-TAP-RAW format */
	downsample = downsample_c64_pal;
#endif

	for (i = DC2N_HEADER_SIZE; i < flen;) {
		utime = (unsigned long) input_buffer[i++];
		utime += 256UL * (unsigned long) input_buffer[i++];

		/* downsample data from 2MHz to C64 PAL frequency */
		clockcycles = downsample(utime);

		/* divide by eight, as requested by TAP V0 format */
		pulse = (clockcycles + 4) / 8;

		/* assert minimal length */
		if (pulse == 0)
			pulse = 1;

		if (pulse < 0x100) {
			/* a pending overflow sequence ends with this pulse */
			if (longpulse) {
				longpulse += pulse;
				olen += write_long_pulse(&output_buffer[olen], longpulse);
				longpulse = 0;
			} else {
				output_buffer[olen++] = (unsigned char) pulse;
			}
		} else if (utime < 0xFFFF) {	/* not a counter overflow, just a long pulse */
			longpulse += pulse;
			olen += write_long_pulse(&output_buffer[olen], longpulse);
			longpulse = 0;
		} else	/* counter overflows get concatenated between them and any following */
			longpulse += pulse;
	}

	/* write trailing longpulse (if any) */
	if (longpulse)
		olen += write_long_pulse(&output_buffer[olen], longpulse);

	/* Fix TAP data size field inside the header */
	output_buffer[0x10] = (unsigned char) ((olen - 20)  & 0xFF);
	output_buffer[0x11] = (unsigned char) (((olen - 20) >>8) & 0xFF);
	output_buffer[0x12] = (unsigned char) (((olen - 20) >>16) & 0xFF);
	output_buffer[0x13] = (unsigned char) (((olen - 20) >>24) & 0xFF);

	return olen;
}
