#ifndef SD_MAIN_H
#define SD_MAIN_H

#include "main.h"
#include "ff.h"


extern FRESULT f_readn (
	FIL *fp, 		/* Pointer to the file object */
	void *buff,		/* Pointer to data buffer */
	UINT btr,		/* Number of bytes to read */
	UINT *br		/* Pointer to number of bytes read */
);

extern FRESULT f_writen (
		FIL *fp,			/* Pointer to the file object */
		const void *buff,	/* Pointer to the data to be written */
		UINT btw,			/* Number of bytes to write */
		UINT *bw			/* Pointer to number of bytes written */
);

extern FRESULT f_findfirst (
	DIR* dp,				/* Pointer to the blank directory object */
	FILINFO* fno,			/* Pointer to the file information structure */
	const TCHAR* path,		/* Pointer to the directory to open */
	const TCHAR* pattern	/* Pointer to the matching pattern */
);

extern FRESULT f_findnext (
	DIR* dp,		/* Pointer to the open directory object */
	FILINFO* fno	/* Pointer to the file information structure */
);

#endif /*SD_MAIN_H*/
