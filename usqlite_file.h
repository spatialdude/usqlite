/*
MIT License

Copyright(c) 2021 Elvin Slavik

Permission is hereby granted, free of charge, to any person obtaining a copy
of this softwareand associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright noticeand this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef usqlite_file_h
#define usqlite_file_h

// ------------------------------------------------------------------------------

#include "py/obj.h"

// ------------------------------------------------------------------------------

#define MAXPATHNAME     1024

typedef struct _MPFILE
{
    sqlite3_file base;
    char pathname[MAXPATHNAME];
    int flags;
    mp_obj_t stream;
}
MPFILE;

// ------------------------------------------------------------------------------

bool usqlite_file_exists(const char *filepath);
int usqlite_file_open(MPFILE *file, const char *name, int flags);
int usqlite_file_close(MPFILE *file);
int usqlite_file_read(MPFILE *file, void *pBuf, int nBuf);
int usqlite_file_write(MPFILE *file, const void *pBuf, int nBuf);
int usqlite_file_flush(MPFILE *file);
int usqlite_file_seek(MPFILE *file, int offset, int origin);
int usqlite_file_tell(MPFILE *file);
int usqlite_file_delete(const char *pathname);

// ------------------------------------------------------------------------------
#endif
