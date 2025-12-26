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

#include "py/objstr.h"
#include "py/objmodule.h"
#include "py/runtime.h"
#include "py/stream.h"
#include "py/builtin.h"

extern const mp_obj_module_t mp_module_io;

// ------------------------------------------------------------------------------

bool usqlite_file_exists(const char* pathname) {
    mp_obj_t os = mp_module_get_builtin(MP_QSTR_uos, 0);
    mp_obj_t ilistdir = usqlite_method(os, MP_QSTR_ilistdir);

    char path[MAXPATHNAME + 1];
    size_t pathname_len = strlen(pathname);

    // Check if pathname fits in buffer
    if (pathname_len >= MAXPATHNAME + 1) {
        return false;  // Path too long
    }

    strcpy(path, pathname);
    const char* filename = pathname;

    char* lastSep = strrchr(path, '/');
    if (lastSep) {
        *lastSep++ = 0;
        filename = lastSep;
    }
    else {
        lastSep = strrchr(path, '\\');
        if (lastSep) {
            *lastSep++ = 0;
            filename = lastSep;
        }
        else {
            path[0] = '.';
            path[1] = 0;
        }
    }

    bool exists = false;
    mp_obj_t listdir = mp_call_function_1(ilistdir, mp_obj_new_str(path, strlen(path)));
    mp_obj_t entry = mp_iternext(listdir);

    while (entry != MP_OBJ_STOP_ITERATION) {
        mp_obj_tuple_t* t = MP_OBJ_TO_PTR(entry);

        int type = mp_obj_get_int(t->items[1]);
        if (type == 0x8000) {
            const char* name = mp_obj_str_get_str(t->items[0]);
            if ((exists = strcmp(filename, name) == 0)) {
                break;
            }
        }

        entry = mp_iternext(listdir);
    }


    return exists;
}

// ------------------------------------------------------------------------------

int usqlite_file_open(MPFILE* file, const char* pathname, int flags) {
    LOGFUNC;

    mp_obj_t filename = mp_obj_new_str(pathname, strlen(pathname));

    char mode[8];
    memset(mode, 0, sizeof(mode));
    char* pMode = mode;

    if (flags & SQLITE_OPEN_CREATE) {
        if (!usqlite_file_exists(pathname)) {
            *pMode++ = 'w';
        }
        else {
            *pMode++ = 'r'; // Open existing file for read/write
        }
        *pMode++ = '+';

    }
    else if (flags & SQLITE_OPEN_READWRITE) {
        *pMode++ = 'r';
        *pMode++ = '+';
    }
    else if (flags & SQLITE_OPEN_READONLY) {
        *pMode++ = 'r';
    }
    else {
        *pMode++ = 'r';
    }

    *pMode++ = 'b';

    mp_obj_t filemode = mp_obj_new_str(mode, strlen(mode));

    usqlite_logprintf(___FUNC___ " '%s' mode:%s\n", pathname, mode);

    mp_obj_t open = usqlite_method(&mp_module_io, MP_QSTR_open);
    file->stream = mp_call_function_2(open, filename, filemode);

    //Copy at most MAXPATHNAME - 1 characters
    strncpy(file->pathname, pathname, MAXPATHNAME - 1);
    // Ensure the very last byte is always null, preventing overrun reads
    file->pathname[MAXPATHNAME - 1] = '\0';

    file->flags = flags;

    // const mp_stream_p_t* stream = mp_get_stream(file->stream);

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

int usqlite_file_close(MPFILE* file) {
    LOGFUNC;

    if (file->stream) {
        usqlite_logprintf(___FUNC___ " %s\n", file->pathname);

        mp_stream_close(file->stream);
        file->stream = NULL;

        if (file->flags & SQLITE_OPEN_DELETEONCLOSE) {
            usqlite_file_delete(file->pathname);
        }
    }

    return SQLITE_OK;
}

// ------------------------------------------------------------------------------

int usqlite_file_read(MPFILE* file, void* pBuf, size_t nBuf) {
    LOGFUNC;

    int error = 0;
    mp_uint_t size = mp_stream_rw(file->stream, pBuf, nBuf, &error, MP_STREAM_RW_READ);
    if (size != nBuf) {
        usqlite_errprintf("write error: %d", error);
    }

    return size;
}

// ------------------------------------------------------------------------------

int usqlite_file_write(MPFILE* file, const void* pBuf, size_t nBuf) {
    LOGFUNC;

    int error = 0;

    mp_uint_t size = mp_stream_rw(file->stream, (void*)pBuf, nBuf, &error, MP_STREAM_RW_WRITE);
    if (size != nBuf) {
        usqlite_errprintf("write error: %d", error);
    }

    return size;
}

// ------------------------------------------------------------------------------

int usqlite_file_flush(MPFILE* file) {
    LOGFUNC;

    const mp_stream_p_t* stream = mp_get_stream(file->stream);

    int error = 0;
    mp_uint_t result = stream->ioctl(file->stream, MP_STREAM_FLUSH, 0, &error);
    if (result == MP_STREAM_ERROR) {
        usqlite_errprintf("flush error: %d", error);

        return error;
    }

    return 0;
}

// ------------------------------------------------------------------------------

int usqlite_file_seek(MPFILE* file, int offset, int origin) {
    LOGFUNC;

    struct mp_stream_seek_t seek;

    seek.offset = offset;
    seek.whence = origin;

    const mp_stream_p_t* stream = mp_get_stream(file->stream);

    int error;
    mp_uint_t result = stream->ioctl(file->stream, MP_STREAM_SEEK, (mp_uint_t)(uintptr_t)&seek, &error);
    if (result == MP_STREAM_ERROR) {
        usqlite_errprintf("seek error: %d", error);
        return -1;
    }

    return seek.offset;
}

// ------------------------------------------------------------------------------

int usqlite_file_tell(MPFILE* file) {
    LOGFUNC;

    return usqlite_file_seek(file, 0, MP_SEEK_CUR);
}

// ------------------------------------------------------------------------------

int usqlite_file_delete(const char* pathname) {
    LOGFUNC;

    usqlite_logprintf("%s: %s\n", __func__, pathname);

    mp_obj_t filename = mp_obj_new_str(pathname, strlen(pathname));
    mp_obj_t remove = usqlite_method(mp_module_get_builtin(MP_QSTR_uos, 0), MP_QSTR_remove);
    mp_call_function_1(remove, filename);

    return SQLITE_OK;
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
