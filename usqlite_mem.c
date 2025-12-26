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

#include "py/runtime.h"
#include "py/obj.h"
#include "py/gc.h"

#include "usqlite.h"

// ------------------------------------------------------------------------------

#if defined(SQLITE_ZERO_MALLOC) && defined(SQLITE_ENABLE_MEMSYS5)

static mp_obj_t sqlite_heap;

// ------------------------------------------------------------------------------

void usqlite_mem_init(void) {
    LOGFUNC;
    // usqlite_logprintf("usqlite_init\n");

    usqlite_logprintf("zero malloc heap: %d\n", MEMSYS5_HEAP_SIZE);

    void* heap = m_malloc(MEMSYS5_HEAP_SIZE);
    if (!heap) {
        mp_raise_msg_varg(&usqlite_Error, MP_ERROR_TEXT("Failed to alloc heap: %d"), MEMSYS5_HEAP_SIZE);
        return;
    }

    LOGLINE;
    sqlite_heap = MP_OBJ_FROM_PTR(heap);
    sqlite3_config(SQLITE_CONFIG_HEAP, heap, MEMSYS5_HEAP_SIZE, 0);
    LOGLINE;
}
#endif


#if defined(SQLITE_ZERO_MALLOC) && !defined(SQLITE_ENABLE_MEMSYS5)
// ------------------------------------------------------------------------------

void* mpmemMalloc(int size) {
    void* mem = gc_alloc(size, false);

#if MICROPY_MEM_STATS
    MP_STATE_MEM(total_bytes_allocated) += size;
    MP_STATE_MEM(current_bytes_allocated) += size;
#endif

    return mem;
}

void mpmemFree(void* mem) {
#if MICROPY_MEM_STATS
    int size = gc_nbytes(mem);
    MP_STATE_MEM(total_bytes_allocated) -= size;
    MP_STATE_MEM(current_bytes_allocated) -= size;
#endif

    gc_free(mem);
}

void* mpmemRealloc(void* mem, int size) {
#if MICROPY_MEM_STATS
    int nsize = size - gc_nbytes(mem);

    MP_STATE_MEM(total_bytes_allocated) += nsize;
    MP_STATE_MEM(current_bytes_allocated) += nsize;
#endif

    return gc_realloc(mem, size, true);
}

int mpmemSize(void* mem) {
    return gc_nbytes(mem);
}

int mpmemRoundup(int size) {
    return size;
}

int mpmemInit(void* appData) {
    LOGFUNC;

    return SQLITE_OK;
}

void mpmemShutdown(void* appData) {
    LOGFUNC;
}

// ------------------------------------------------------------------------------

sqlite3_mem_methods mpmem = {
    mpmemMalloc,    /* Memory allocation function */
    mpmemFree,      /* Free a prior allocation */
    mpmemRealloc,   /* Resize an allocation */
    mpmemSize,      /* Return the size of an allocation */
    mpmemRoundup,   /* Round up request size to allocation size */
    mpmemInit,      /* Initialize the memory allocator */
    mpmemShutdown,  /* Deinitialize the memory allocator */
    NULL            /* Argument to xInit() and xShutdown() */
};

// ------------------------------------------------------------------------------

void usqlite_mem_init(void) {
    LOGFUNC;
    // usqlite_logprintf("usqlite_init\n");

    usqlite_logprintf("MicroPython memory management\n");

    sqlite3_config(SQLITE_CONFIG_MALLOC, &mpmem);
}

// ------------------------------------------------------------------------------
#endif
