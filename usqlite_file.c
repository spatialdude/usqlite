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

#include <stdio.h>

#include "py/objstr.h"
#include "py/objmodule.h"
#include "py/objlist.h"
#include "py/runtime.h"
#include "py/gc.h"
#include "py/stream.h"
#include "py/builtin.h"
#include "py/mphal.h"

extern const mp_obj_module_t mp_module_io;

// ------------------------------------------------------------------------------

// Every open database file is a MicroPython stream object (io.open()), but the
// only pointer to it lives inside SQLite's sqlite3_file, which SQLite allocates
// from its own dedicated heap. The GC does not walk SQLite's heap as object
// memory, so without a strong reference here the stream can be collected out
// from under an open connection -- a use-after-free that surfaces only after a
// gc.collect(), and only on some memory layouts. Pin every open stream in a
// GC-visible list held from a root pointer; unpin on close.
MP_REGISTER_ROOT_POINTER(mp_obj_t usqlite_files);

static void usqlite_file_pin(mp_obj_t stream) {
    if (!MP_STATE_VM(usqlite_files)) {
        MP_STATE_VM(usqlite_files) = mp_obj_new_list(0, NULL);
    }
    mp_obj_list_append(MP_STATE_VM(usqlite_files), stream);
}

static void usqlite_file_unpin(mp_obj_t stream) {
    mp_obj_t lst = MP_STATE_VM(usqlite_files);
    if (!lst) {
        return;
    }
    // Swap-remove by identity; never raises (mp_obj_list_remove would, and this
    // runs on the close/finalize path where a raise must not happen).
    mp_obj_list_t *l = MP_OBJ_TO_PTR(lst);
    for (size_t i = 0; i < l->len; i++) {
        if (l->items[i] == stream) {
            l->items[i] = l->items[l->len - 1];
            l->items[l->len - 1] = MP_OBJ_NULL;
            l->len--;
            return;
        }
    }
}

// ------------------------------------------------------------------------------

// os.stat probe: one name lookup instead of the old ilistdir() directory walk
// (which was slower, raised through SQLite's frames when the directory was
// missing, and mis-resolved root-level names to the cwd). Never raises. Now
// also on the hot path: hot-journal detection probes existence via xAccess.
bool usqlite_file_exists(const char *pathname) {
    return usqlite_file_accessible(pathname);
}

// ------------------------------------------------------------------------------

// True if os.stat(pathname) succeeds -- used by the VFS xAccess to answer
// SQLite's "is this a writable directory?" probe (temp_store_directory). Guarded
// by nlr because os.stat raises rather than returning a code when absent.
bool usqlite_file_accessible(const char *pathname) {
    if (!pathname) {
        return false;
    }
    nlr_buf_t nlr;
    bool ok = false;
    if (nlr_push(&nlr) == 0) {
        mp_obj_t os = mp_module_get_builtin(MP_QSTR_uos, 0);
        mp_obj_t stat = usqlite_method(os, MP_QSTR_stat);
        mp_call_function_1(stat, mp_obj_new_str(pathname, strlen(pathname)));
        nlr_pop();
        ok = true;
    }
    return ok;
}

// ------------------------------------------------------------------------------

int usqlite_file_open(MPFILE *file, const char *pathname, int flags) {
    LOGFUNC;

    // SQLite requests anonymous temporary files (sorter/merge spill, temp
    // b-trees) by passing a NULL name and expecting the VFS to invent one --
    // dereferencing that NULL previously crashed the board. Give temp files a
    // unique name in the temp directory so large sorts and index builds spill
    // to disk instead of faulting. PRAGMA temp_store_directory overrides the
    // default (USQLITE_TEMP_DIR).
    char tmpname[128];
    bool is_temp = (pathname == NULL) ||
        ((flags & (SQLITE_OPEN_TEMP_DB | SQLITE_OPEN_TEMP_JOURNAL |
                   SQLITE_OPEN_TRANSIENT_DB | SQLITE_OPEN_SUBJOURNAL)) != 0);
    if (pathname == NULL) {
        static uint32_t seq;
        // Prefer the SD card when it is mounted (spares flash the spill churn),
        // fall back to flash; PRAGMA temp_store_directory overrides both.
        const char *dir =
            sqlite3_temp_directory ? sqlite3_temp_directory :
            usqlite_file_accessible(USQLITE_TEMP_SD) ? USQLITE_TEMP_SD :
            USQLITE_TEMP_DIR;
        size_t dlen = strlen(dir);
        const char *sep = (dlen && dir[dlen - 1] == '/') ? "" : "/";
        snprintf(tmpname, sizeof(tmpname), "%s%setilqs_%08x%08x",
            dir, sep, (unsigned)mp_hal_ticks_ms(), (unsigned)seq++);
        pathname = tmpname;
    }

    char mode[8];
    memset(mode, 0, sizeof(mode));
    char *pMode = mode;

    if (flags & SQLITE_OPEN_CREATE) {
        // A temp file has a fresh unique name (and root-level paths confuse the
        // existence probe), so skip the check and always create-new.
        if (is_temp || !usqlite_file_exists(pathname)) {
            *pMode++ = 'w';
        }

        *pMode++ = '+';
    } else if (flags & SQLITE_OPEN_READWRITE) {
        *pMode++ = 'r';
        *pMode++ = '+';
    } else if (flags & SQLITE_OPEN_READONLY) {
        *pMode++ = 'r';
    } else {
        *pMode++ = 'r';
    }

    *pMode++ = 'b';

    usqlite_logprintf(___FUNC___ " '%s' mode:%s\n", pathname, mode);

    // io.open raises on failure (missing directory, absent or full card, ...).
    // A raise here would unwind SQLite mid-operation, leaking engine state --
    // convert it to the error code the VFS contract expects instead.
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_obj_t filename = mp_obj_new_str(pathname, strlen(pathname));
        mp_obj_t filemode = mp_obj_new_str(mode, strlen(mode));
        mp_obj_t open = usqlite_method(&mp_module_io, MP_QSTR_open);
        file->stream = mp_call_function_2(open, filename, filemode);
        usqlite_file_pin(file->stream);
        nlr_pop();
    } else {
        file->stream = NULL;
        return SQLITE_CANTOPEN;
    }

    strcpy(file->pathname, pathname);
    file->flags = flags;

    return SQLITE_OK;
}

/*
mp_obj_t args[2] =
{
    filename,
    filemode
};

file->stream = mp_builtin_open(2, args, NULL);
*/

// ------------------------------------------------------------------------------

int usqlite_file_close(MPFILE *file) {
    LOGFUNC;

    if (file->stream) {
        usqlite_logprintf(___FUNC___ " %s\n", file->pathname);

        // A stream-close error must not unwind through SQLite's teardown;
        // whatever happens, finish the job -- unpin and forget the stream.
        nlr_buf_t nlr;
        if (nlr_push(&nlr) == 0) {
            mp_stream_close(file->stream);
            nlr_pop();
        }
        usqlite_file_unpin(file->stream);
        file->stream = NULL;

        if (file->flags & SQLITE_OPEN_DELETEONCLOSE) {
            usqlite_file_delete(file->pathname);
        }
    }

    return SQLITE_OK;
}

// ------------------------------------------------------------------------------

// Truncate the file to zero length. MicroPython streams have no truncate
// ioctl, so emulate by reopening the file with a truncating mode. SQLite
// calls this on every commit (locking_mode=EXCLUSIVE finalizes the rollback
// journal with a truncate instead of a delete); if the old journal content
// survived a "successful" truncate, the next open after a power cut would
// replay a stale journal over committed data.
int usqlite_file_truncate0(MPFILE *file) {
    LOGFUNC;

    if (!file->stream) {
        return SQLITE_IOERR_TRUNCATE;
    }

    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_stream_close(file->stream);
        usqlite_file_unpin(file->stream);
        file->stream = NULL;

        mp_obj_t filename = mp_obj_new_str(file->pathname, strlen(file->pathname));
        mp_obj_t filemode = mp_obj_new_str("w+b", 3);
        mp_obj_t open = usqlite_method(&mp_module_io, MP_QSTR_open);
        file->stream = mp_call_function_2(open, filename, filemode);
        usqlite_file_pin(file->stream);
        nlr_pop();
        return SQLITE_OK;
    } else {
        return SQLITE_IOERR_TRUNCATE;
    }
}

// ------------------------------------------------------------------------------

int usqlite_file_read(MPFILE *file, void *pBuf, size_t nBuf) {
    LOGFUNC;

    int error = 0;
    mp_uint_t size = mp_stream_rw(file->stream, pBuf, nBuf, &error, MP_STREAM_RW_READ);
    if (size != nBuf) {
        usqlite_errprintf("read error: %d", error);
    }

    return size;
}

// ------------------------------------------------------------------------------

int usqlite_file_write(MPFILE *file, const void *pBuf, size_t nBuf) {
    LOGFUNC;

    int error = 0;

    mp_uint_t size = mp_stream_rw(file->stream, (void *)pBuf, nBuf, &error, MP_STREAM_RW_WRITE);
    if (size != nBuf) {
        usqlite_errprintf("write error: %d", error);
    }

    return size;
}

// ------------------------------------------------------------------------------

int usqlite_file_flush(MPFILE *file) {
    LOGFUNC;

    const mp_stream_p_t *stream = mp_get_stream(file->stream);

    int error = 0;
    mp_uint_t result = stream->ioctl(file->stream, MP_STREAM_FLUSH, 0, &error);
    if (result == MP_STREAM_ERROR) {
        usqlite_errprintf("flush error: %d", error);

        return error;
    }

    return 0;
}

// ------------------------------------------------------------------------------

int usqlite_file_seek(MPFILE *file, int offset, int origin) {
    LOGFUNC;

    struct mp_stream_seek_t seek;

    seek.offset = offset;
    seek.whence = origin;

    const mp_stream_p_t *stream = mp_get_stream(file->stream);

    int error;
    mp_uint_t result = stream->ioctl(file->stream, MP_STREAM_SEEK, (mp_uint_t)(uintptr_t)&seek, &error);
    if (result == MP_STREAM_ERROR) {
        usqlite_errprintf("seek error: %d", error);
        return -1;
    }

    return seek.offset;
}

// ------------------------------------------------------------------------------

int usqlite_file_tell(MPFILE *file) {
    LOGFUNC;

    return usqlite_file_seek(file, 0, MP_SEEK_CUR);
}

// ------------------------------------------------------------------------------

int usqlite_file_delete(const char *pathname) {
    LOGFUNC;

    usqlite_logprintf("%s: %s\n", __func__, pathname);

    // This path allocates Python objects to call os.remove(). During a GC
    // sweep (finaliser-driven close, e.g. at soft reset) the heap is locked
    // and that would raise MemoryError out through SQLite's C frames,
    // aborting sqlite3_close halfway. Report failure instead; a leftover
    // journal file is harmless here (hot-journal recovery is not enabled).
    if (gc_is_locked()) {
        return SQLITE_IOERR_DELETE;
    }

    // os.remove raises when the file is already gone or the card was pulled;
    // report failure as a code rather than unwinding SQLite's C frames.
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_obj_t filename = mp_obj_new_str(pathname, strlen(pathname));
        mp_obj_t remove = usqlite_method(mp_module_get_builtin(MP_QSTR_uos, 0), MP_QSTR_remove);
        mp_call_function_1(remove, filename);
        nlr_pop();
        return SQLITE_OK;
    } else {
        return SQLITE_IOERR_DELETE;
    }
}

// ------------------------------------------------------------------------------
/*
static mp_obj_t fileIoctl(MPFILE* file, size_t n_args, const mp_obj_t* args)
{
    mp_buffer_info_t bufinfo;
    uintptr_t val = 0;
    if (n_args > 2) {
        if (mp_get_buffer(args[2], &bufinfo, MP_BUFFER_WRITE)) {
            val = (uintptr_t)bufinfo.buf;
        }
        else {
            val = mp_obj_get_int_truncated(args[2]);
        }
    }

    const mp_stream_p_t* stream_p = mp_get_stream(args[0]);
    int error;
    mp_uint_t res = stream_p->ioctl(args[0], mp_obj_get_int(args[1]), val, &error);
    if (res == MP_STREAM_ERROR) {
        mp_raise_OSError(error);
    }

    return mp_obj_new_int(res);
}
*/

// ------------------------------------------------------------------------------
