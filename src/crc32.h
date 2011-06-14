
/* crc32.h */

#define CRC32_POLYNOMIAL	0xEDB88320L

int build_crc_table(void);
unsigned /*long*/ int compute_crc32(unsigned char *, int);
void free_crc_table(void);

