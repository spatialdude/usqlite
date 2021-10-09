#ifndef usqlite_file_h
#define usqlite_file_h

//------------------------------------------------------------------------------

#include "sqlite3.h"

#include "py/obj.h"

//------------------------------------------------------------------------------

#define MAXPATHNAME     1024

typedef struct _MPFILE
{
    sqlite3_file    base;
    char            pathname[MAXPATHNAME];
    int             flags;
    mp_obj_t        stream;
}
MPFILE;

//------------------------------------------------------------------------------

int usqlite_file_open(MPFILE* file, const char* name, int flags);
int usqlite_file_close(MPFILE* file);
int usqlite_file_read(MPFILE* file, void* pBuf, int nBuf);
int usqlite_file_write(MPFILE* file, const void* pBuf, int nBuf);
int usqlite_file_flush(MPFILE* file);
int usqlite_file_seek(MPFILE* file, int offset, int origin);
int usqlite_file_tell(MPFILE* file);
int usqlite_file_delete(const char* pathname);

//------------------------------------------------------------------------------
#endif
