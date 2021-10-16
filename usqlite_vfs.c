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

#include "usqlite.h"

#include <stdarg.h>
#include <stdlib.h>

#include "py/objstr.h"
#include "py/runtime.h"
#include "py/stream.h"
#include "py/builtin.h"

//------------------------------------------------------------------------------

static sqlite3_vfs mpvfs;

int sqlite3_os_init(void)
{
    LOGFUNC;

    //usqlite_logprintf("sqlite3_os_init\n");

    int rc = sqlite3_vfs_register(&mpvfs, 1);

    return rc;
}

//------------------------------------------------------------------------------

int sqlite3_os_end(void)
{
    LOGFUNC;

    //usqlite_logprintf("sqlite3_os_end\n");

    return SQLITE_OK;
}

//------------------------------------------------------------------------------

static int mpvfsClose(sqlite3_file* pFile)
{
    LOGFUNC;

    return usqlite_file_close((MPFILE*)pFile);
}

//------------------------------------------------------------------------------

static int mpvfsRead(sqlite3_file* pFile, void* pBuf, int nBuf, sqlite3_int64 offset)
{
    LOGFUNC;

    MPFILE* file = (MPFILE*)pFile;

    if (usqlite_file_seek(file, offset, MP_SEEK_SET) != offset)
    {
        return SQLITE_ERROR;
    }

    int size = usqlite_file_read(file, pBuf, nBuf);

    return size == nBuf
        ? SQLITE_OK
        : SQLITE_IOERR_SHORT_READ;
}

//------------------------------------------------------------------------------

static int mpvfsWrite(sqlite3_file* pFile, const void* pBuf, int nBuf, sqlite3_int64 offset)
{
    LOGFUNC;

    MPFILE* file = (MPFILE*)pFile;

    if (usqlite_file_seek(file, offset, MP_SEEK_SET) != offset)
    {
        return SQLITE_ERROR;
    }

    int size = usqlite_file_write(file, pBuf, nBuf);
    return size == nBuf
        ? SQLITE_OK
        : SQLITE_IOERR_WRITE;
}

//------------------------------------------------------------------------------

static int mpvfsTruncate(sqlite3_file* pFile, sqlite3_int64 size)
{
    LOGFUNC;
    
#ifdef USQLITE_DEBUG
    MPFILE* file = (MPFILE*)pFile;

    usqlite_logprintf(___FUNC___ "%s\n", file->pathname);
#endif

    return SQLITE_OK;
}

//------------------------------------------------------------------------------

static int mpvfsSync(sqlite3_file* pFile, int flags)
{
    LOGFUNC;

    return usqlite_file_flush((MPFILE*)pFile)
        ? SQLITE_ERROR
        : SQLITE_OK;
}

//------------------------------------------------------------------------------

static int mpvfsFileSize(sqlite3_file* pFile, sqlite3_int64* pSize)
{
    LOGFUNC;

    MPFILE* file = (MPFILE*)pFile;

    int offset = usqlite_file_tell(file);
    if (offset < 0)
    {
        return SQLITE_ERROR;
    }

    int size = usqlite_file_seek(file, 0, MP_SEEK_END);
    if (size < 0)
    {
        return SQLITE_ERROR;
    }

    *pSize = size;

    offset = usqlite_file_seek(file, offset, MP_SEEK_SET);
    return offset < 0
        ? SQLITE_ERROR
        : SQLITE_OK;
}

//------------------------------------------------------------------------------

static int mpvfsLock(sqlite3_file* pFile, int eLock)
{
    LOGFUNC;

    return SQLITE_OK;
}

//------------------------------------------------------------------------------

static int mpvfsUnlock(sqlite3_file* pFile, int eLock)
{
    LOGFUNC;

    return SQLITE_OK;
}

//------------------------------------------------------------------------------

static int mpvfsCheckReservedLock(sqlite3_file* pFile, int* pResOut)
{
    LOGFUNC;

    *pResOut = 0;

    return SQLITE_OK;
}

//------------------------------------------------------------------------------
 
static int mpvfsFileControl(sqlite3_file* pFile, int op, void* pArg)
{
    LOGFUNC;

    return SQLITE_NOTFOUND;
}

//------------------------------------------------------------------------------

static int mpvfsSectorSize(sqlite3_file* pFile)
{
    LOGFUNC;

    return 0;
}

//------------------------------------------------------------------------------

static int mpvfsDeviceCharacteristics(sqlite3_file* pFile)
{
    LOGFUNC;

    return 0;
}

//------------------------------------------------------------------------------
/*  int (*xShmMap)(sqlite3_file*, int iPg, int pgsz, int, void volatile**);
  int (*xShmLock)(sqlite3_file*, int offset, int n, int flags);
  void (*xShmBarrier)(sqlite3_file*);
  int (*xShmUnmap)(sqlite3_file*, int deleteFlag);
  int (*xFetch)(sqlite3_file*, sqlite3_int64 iOfst, int iAmt, void **pp);
  int (*xUnfetch)(sqlite3_file*, sqlite3_int64 iOfst, void *p);
*/

static sqlite3_io_methods mpvfs_io_methods =
{
    3,                                  /* iVersion */
    mpvfsClose,                         /* xClose */
    mpvfsRead,                          /* xRead */
    mpvfsWrite,                         /* xWrite */
    mpvfsTruncate,                      /* xTruncate */
    mpvfsSync,                          /* xSync */
    mpvfsFileSize,                      /* xFileSize */
    mpvfsLock,                          /* xLock */
    mpvfsUnlock,                        /* xUnlock */
    mpvfsCheckReservedLock,             /* xCheckReservedLock */
    mpvfsFileControl,                   /* xFileControl */
    mpvfsSectorSize,                    /* xSectorSize */
    mpvfsDeviceCharacteristics,         /* xDeviceCharacteristics */
    //mpvfsShmMap,                     /* xShmMap */
    //mpvfsShmLock,                    /* xShmLock */
    //mpvfsShmBarrier,                 /* xShmBarrier */
    //mpvfsShmUnmap,                   /* xShmUnmap */
    //mpvfsFetch,
    //mpvfsUnfetch
};

//------------------------------------------------------------------------------

static int mpvfsDelete(sqlite3_vfs* vfs, const char* zName, int syncDir)
{
    LOGFUNC;

    return usqlite_file_delete(zName);
}

//------------------------------------------------------------------------------

static int mpvfsAccess(sqlite3_vfs* vfs, const char *zName, int flags, int *pResOut)
{
//    int rc;                         /* access() return code */
//    int eAccess = F_OK;             /* Second argument to access() */

//    assert(flags == SQLITE_ACCESS_EXISTS       /* access(zPath, F_OK) */
//        || flags == SQLITE_ACCESS_READ         /* access(zPath, R_OK) */
//        || flags == SQLITE_ACCESS_READWRITE    /* access(zPath, R_OK|W_OK) */
//    );

//    if (flags == SQLITE_ACCESS_READWRITE) eAccess = R_OK | W_OK;
//    if (flags == SQLITE_ACCESS_READ)      eAccess = R_OK;

//    rc = access(zPath, eAccess);
    *pResOut = 0;// (rc == 0);

    return SQLITE_OK;
}
//------------------------------------------------------------------------------

static int mpvfsFullPathname(sqlite3_vfs* vfs, const char* zName, int nOut, char* zOut)
{
    LOGFUNC;

    strcpy(zOut, zName);

    return SQLITE_OK;
}

//------------------------------------------------------------------------------

  /*
  void *(*xDlOpen)(sqlite3_vfs*, const char *zFilename);
  void (*xDlError)(sqlite3_vfs*, int nByte, char *zErrMsg);
  void (*(*xDlSym)(sqlite3_vfs*,void*, const char *zSymbol))(void);
  void (*xDlClose)(sqlite3_vfs*, void*);
  */

  //------------------------------------------------------------------------------

static int mpvfsRandomness(sqlite3_vfs* pVfs, int nByte, char* zByte) 
{
    LOGFUNC;

    return SQLITE_OK;
}

//------------------------------------------------------------------------------
/*
  int (*xSleep)(sqlite3_vfs*, int microseconds);
  int (*xCurrentTime)(sqlite3_vfs*, double*);
  int (*xGetLastError)(sqlite3_vfs*, int, char *);
  int (*xCurrentTimeInt64)(sqlite3_vfs*, sqlite3_int64*);
  int (*xSetSystemCall)(sqlite3_vfs*, const char *zName, sqlite3_syscall_ptr);
  sqlite3_syscall_ptr (*xGetSystemCall)(sqlite3_vfs*, const char *zName);
  const char *(*xNextSystemCall)(sqlite3_vfs*, const char *zName);
  */

//------------------------------------------------------------------------------

static int mpvfsOpen(sqlite3_vfs* vfs, const char* zName, sqlite3_file* pFile, int flags, int* pOutFlags)
{
    LOGFUNC;

    MPFILE* file = (MPFILE*)pFile;

    memset(file, 0, sizeof(MPFILE));
    pFile->pMethods = &mpvfs_io_methods;

    if (usqlite_file_open(file, zName, flags))
    {
        return SQLITE_CANTOPEN;
    }

    if (pOutFlags)
    {
        *pOutFlags = 0;
    }

    return SQLITE_OK;
}

//------------------------------------------------------------------------------

static sqlite3_vfs mpvfs = 
{
    1,                              /* iVersion */
    sizeof(MPFILE),                 /* szOsFile */
    MAXPATHNAME,                    /* mxPathname */
    NULL,                           /* pNext */
    "mpvfs",                        /* zName */
    NULL,                           /* pAppData */
    mpvfsOpen,                      /* xOpen */
    mpvfsDelete,                    /* xDelete */
    mpvfsAccess,                    /* xAccess */
    mpvfsFullPathname,              /* xFullPathname */
    NULL,//mpvfsDlOpen,                   /* xDlOpen */
    NULL,//mpvfsDlError,                  /* xDlError */
    NULL,//mpvfsDlSym,                    /* xDlSym */
    NULL,//mpvfsDlClose,                  /* xDlClose */
    mpvfsRandomness,                /* xRandomness */
    NULL,//mpvfsSleep,                    /* xSleep */
    NULL,//mpvfsCurrentTime,              /* xCurrentTime */
};

//------------------------------------------------------------------------------


