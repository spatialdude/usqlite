//------------------------------------------------------------------------------

#include "sqlite3.h"
#include "usqlite_file.h"
#include "usqlite_utils.h"

#include <stdarg.h>

#include "py/objstr.h"
#include "py/runtime.h"
#include "py/stream.h"
#include "py/builtin.h"

//------------------------------------------------------------------------------

static int mpvfsClose(sqlite3_file* pFile)
{
    return usqlite_file_close((MPFILE*)pFile);
}

//------------------------------------------------------------------------------

static int mpvfsRead(sqlite3_file* pFile, void* pBuf, int nBuf, sqlite3_int64 offset)
{
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
    MPFILE* file = (MPFILE*)pFile;

    usqlite_logprintf(__FUNCDNAME__ "%s\n", file->pathname);

    return SQLITE_OK;
}

//------------------------------------------------------------------------------

static int mpvfsSync(sqlite3_file* pFile, int flags)
{
    return usqlite_file_flush((MPFILE*)pFile)
        ? SQLITE_ERROR
        : SQLITE_OK;
}

//------------------------------------------------------------------------------

static int mpvfsFileSize(sqlite3_file* pFile, sqlite3_int64* pSize)
{
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
    return SQLITE_OK;
}

//------------------------------------------------------------------------------

static int mpvfsUnlock(sqlite3_file* pFile, int eLock)
{
    return SQLITE_OK;
}

//------------------------------------------------------------------------------

static int mpvfsCheckReservedLock(sqlite3_file* pFile, int* pResOut)
{
    *pResOut = 0;

    return SQLITE_OK;
}

//------------------------------------------------------------------------------
 
static int mpvfsFileControl(sqlite3_file* pFile, int op, void* pArg)
{
    return SQLITE_NOTFOUND;
}

//------------------------------------------------------------------------------

static int mpvfsSectorSize(sqlite3_file* pFile)
{
    return 0;
}

//------------------------------------------------------------------------------

static int mpvfsDeviceCharacteristics(sqlite3_file* pFile)
{
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

int sqlite3_os_init(void)
{
    usqlite_logprintf("sqlite3_os_init\n");

    int rc = sqlite3_vfs_register(&mpvfs, 1);

    return rc;
}

//------------------------------------------------------------------------------

int sqlite3_os_end(void)
{
    usqlite_logprintf("sqlite3_os_end\n");
    
    return SQLITE_OK;
}

//------------------------------------------------------------------------------

/*

#if MICROPY_VFS == 1
#include "extmod/vfs.h"
#endif
#if MICROPY_VFS_POSIX_FILE == 1
#include "extmod/vfs_posix.h"
#endif

#if MICROPY_VFS_POSIX_FILE == 11
    mpFile->stream = mp_vfs_posix_file_open(
        &mp_type_vfs_posix_fileio,
        mp_obj_new_str(zName, strlen(zName)),
        mp_obj_new_str("w+b", 3));
    //        MP_OBJ_NEW_QSTR(MP_QSTR_wrb));
#endif
#if MICROPY_VFS == 1
    mp_obj_t args[2] =
    {
        mp_obj_new_str(filename, strlen(filename)),
        MP_OBJ_NEW_QSTR(MP_QSTR_wb),
    };

    //    mp_obj_t file = mp_vfs_open(MP_ARRAY_SIZE(args), &args[0], (mp_map_t *)&mp_const_empty_map);
#endif


//    char* filename = "file.txt";


#if MICROPY_VFS_POSIX_FILE == 11
    mp_obj_t file = mp_vfs_posix_file_open(
        &mp_type_vfs_posix_fileio,
        mp_obj_new_str(filename, strlen(filename)),
        MP_OBJ_NEW_QSTR(MP_QSTR_wb));
#endif
#if MICROPY_VFS == 11
    mp_obj_t args[2] =
    {
        mp_obj_new_str(filename, strlen(filename)),
        MP_OBJ_NEW_QSTR(MP_QSTR_wb),
    };

    //    mp_obj_t file = mp_vfs_open(MP_ARRAY_SIZE(args), &args[0], (mp_map_t *)&mp_const_empty_map);
#endif

    //mp_stream_write(file, "0123456789", 10, MP_STREAM_RW_WRITE);

    //mp_stream_close(file);

//    int result = sqlite3_initialize();
//    if (result)
//    {
//        errprintf("sqlite3_initialize error %d\n", result);
//    }

//    sqlite3* db = NULL;
//    int result = sqlite3_open("test.sqlite", &db);

//    return mp_obj_new_bool(1);// !result);

//    return MP_OBJ_NULL;
}


*/

