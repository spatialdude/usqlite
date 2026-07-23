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

// The one dedicated heap for the whole SQLite engine, handed to SQLite via
// SQLITE_CONFIG_HEAP so every SQLite allocation lives inside it. To the
// MicroPython GC this is a single block: it never sees, and so never frees,
// SQLite's internal allocations -- which is what makes gc.collect() (explicit
// or automatic) safe while a connection is open. Held in a GC root so the
// collector keeps the block for the session; a soft reset clears the root and
// hands the RAM back.
MP_REGISTER_ROOT_POINTER(void *usqlite_heap);

// ------------------------------------------------------------------------------

void usqlite_mem_init(void) {
    LOGFUNC;

    // Reserve lazily and once. usqlite_mem_init() runs from the module's
    // initialize(), which fires on the first connect() -- so a program that
    // never opens a database reserves nothing.
    if (MP_STATE_VM(usqlite_heap)) {
        return;
    }

    void *heap = m_malloc_maybe(MEMSYS5_HEAP_SIZE);
    if (!heap) {
        mp_raise_msg_varg(&usqlite_Error,
            MP_ERROR_TEXT("cannot reserve %d bytes for the SQLite engine"),
            MEMSYS5_HEAP_SIZE);
    }

    MP_STATE_VM(usqlite_heap) = heap;
    // Third arg is MEMSYS5's minimum allocation (atom) size: it must be a sane
    // power of two, not 0 -- 0 becomes a 1-byte atom and cripples the buddy
    // allocator with control overhead. 64 bytes is in SQLite's recommended range.
    sqlite3_config(SQLITE_CONFIG_HEAP, heap, MEMSYS5_HEAP_SIZE, 64);
}
#endif


#if defined(SQLITE_ZERO_MALLOC) && !defined(SQLITE_ENABLE_MEMSYS5)
// ------------------------------------------------------------------------------

void *mpmemMalloc(int size) {
    void *mem = gc_alloc(size, false);

    #if MICROPY_MEM_STATS
    MP_STATE_MEM(total_bytes_allocated) += size;
    MP_STATE_MEM(current_bytes_allocated) += size;
    #endif

    return mem;
}

void mpmemFree(void *mem) {
    #if MICROPY_MEM_STATS
    int size = gc_nbytes(mem);
    MP_STATE_MEM(total_bytes_allocated) -= size;
    MP_STATE_MEM(current_bytes_allocated) -= size;
    #endif

    gc_free(mem);
}

void *mpmemRealloc(void *mem, int size) {
    #if MICROPY_MEM_STATS
    int nsize = size - gc_nbytes(mem);

    MP_STATE_MEM(total_bytes_allocated) += nsize;
    MP_STATE_MEM(current_bytes_allocated) += nsize;
    #endif

    return gc_realloc(mem, size, true);
}

int mpmemSize(void *mem) {
    return gc_nbytes(mem);
}

int mpmemRoundup(int size) {
    return size;
}

int mpmemInit(void *appData) {
    LOGFUNC;

    return SQLITE_OK;
}

void mpmemShutdown(void *appData) {
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
