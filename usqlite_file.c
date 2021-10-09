//------------------------------------------------------------------------------

#include "usqlite_file.h"
#include "usqlite_utils.h"

#include "py/objstr.h"
#include "py/runtime.h"
#include "py/stream.h"
#include "py/builtin.h"

//------------------------------------------------------------------------------

int usqlite_file_open(MPFILE* file, const char* pathname, int flags)
{
    char mode[8];
    memset(mode, 0, sizeof(mode));
    char* pMode = mode;

    if (flags & SQLITE_OPEN_CREATE)
    {
        *pMode++ = 'a';
        *pMode++ = '+';
    }
    else if (flags & SQLITE_OPEN_READWRITE)
    {
        *pMode++ = 'r';
        *pMode++ = '+';
    }
    else if (flags & SQLITE_OPEN_READONLY)
    {
        *pMode++ = 'r';
    }
    else
    {
        *pMode++ = 'r';
    }

    *pMode++ = 'b';

    mp_obj_t filename = mp_obj_new_str(pathname, strlen(pathname));
    mp_obj_t filemode = mp_obj_new_str(mode, strlen(mode));

    usqlite_logprintf(__FUNCDNAME__ " '%s' mode:%s\n", pathname, mode);

    mp_obj_t open = usqlite_method(&mp_module_io, MP_QSTR_open);
    file->stream = mp_call_function_2(open, filename, filemode);
    strcpy(file->pathname, pathname);
    file->flags = flags;

    const mp_stream_p_t* stream = mp_get_stream(file->stream);

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

//------------------------------------------------------------------------------

int usqlite_file_close(MPFILE* file)
{
    if (file->stream)
    {
        usqlite_logprintf(__FUNCDNAME__ " %s\n", file->pathname);

        mp_stream_close(file->stream);
        file->stream = NULL;

        if (file->flags & SQLITE_OPEN_DELETEONCLOSE)
        {
            usqlite_file_delete(file->pathname);
        }
    }

    return SQLITE_OK;
}

//------------------------------------------------------------------------------

int usqlite_file_read(MPFILE* file, void* pBuf, int nBuf)
{
    int error = 0;
    mp_uint_t size = mp_stream_rw(file->stream, pBuf, nBuf, &error, MP_STREAM_RW_READ);
    if (size < 0)
    {
        usqlite_errprintf("write error: %d", error);
    }

    return size;
}

//------------------------------------------------------------------------------

int usqlite_file_write(MPFILE* file, const void* pBuf, int nBuf)
{
    int error = 0;

    mp_uint_t size = mp_stream_rw(file->stream, (void*)pBuf, nBuf, &error, MP_STREAM_RW_WRITE);
    if (size < 0)
    {
        usqlite_errprintf("write error: %d", error);
    }

    return size;
}

//------------------------------------------------------------------------------

int usqlite_file_flush(MPFILE* file)
{
    const mp_stream_p_t* stream = mp_get_stream(file->stream);

    int error;
    mp_uint_t result = stream->ioctl(file->stream, MP_STREAM_FLUSH, 0, &error);
    if (result == MP_STREAM_ERROR)
    {
        usqlite_errprintf("flush error: %d", error);

        return error;
    }

    return 0;
}

//------------------------------------------------------------------------------

int usqlite_file_seek(MPFILE* file, int offset, int origin)
{
    struct mp_stream_seek_t seek;

    seek.offset = offset;
    seek.whence = origin;

    const mp_stream_p_t* stream = mp_get_stream(file->stream);

    int error;
    mp_uint_t result = stream->ioctl(file->stream, MP_STREAM_SEEK, (mp_uint_t)(uintptr_t)&seek, &error);
    if (result == MP_STREAM_ERROR)
    {
        usqlite_errprintf("seek error: %d", error);
        return -1;
    }

    return seek.offset;
}

//------------------------------------------------------------------------------

int usqlite_file_tell(MPFILE* file)
{
    return usqlite_file_seek(file, 0, MP_SEEK_CUR);
}

//------------------------------------------------------------------------------

int usqlite_file_delete(const char* pathname)
{
    usqlite_logprintf(__FUNCDNAME__ " delete: %s\n", pathname);

    mp_obj_t filename = mp_obj_new_str(pathname, strlen(pathname));
    mp_obj_t remove = usqlite_method(&mp_module_os, MP_QSTR_remove);
    mp_call_function_1(remove, filename);

    return SQLITE_OK;
}

//------------------------------------------------------------------------------

static mp_obj_t fileIoctl(MPFILE* file, size_t n_args, const mp_obj_t* args)
{
    /*
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
    */
}

//------------------------------------------------------------------------------

