#ifndef FSYS_H 
#define FSYS_H

#include "ff.h"
#include "vmapi.h"
#include "abstract.h"
#include "iterable.h"
#include <string.h>

#if _USE_LFN
#define  FSYS_USE_LFN
#define FSYS_NBUF _MAX_LFN /*max name length*/
#else
#define FSYS_NBUF (12)
#endif

#define FSYS_PBUF 64 /*max path length*/
#define FSYS_EXT 4 /*extension length*/

typedef FIL file_t;
typedef DRESULT ferr_t;

enum {
    FS_NONE,
    FS_ERROR,
    FS_OPEN_READ,
    FS_OPEN_WRITE,
    FS_OPEN_RW,
    FS_NEW,
    FS_CLOSED,
    
};

class File {
    private :
        char name[FSYS_NBUF];
        char path[FSYS_PBUF];
        char ext[FSYS_EXT];
        file_t file;
        uint8_t status;
        uint32_t lock;
    public :
        File (char *path);
        ~File ();

        ferr_t open (char *flags);
        ferr_t close ();

        ferr_t read (void *data, uint32_t size, uint32_t *bytes_read);
        ferr_t write (void *data, uint32_t size, uint32_t *bytes_write);
        
        
        static const char Separator = '/';
        static const char Dot = '.' ;
};

#endif /*FSYS_H*/
