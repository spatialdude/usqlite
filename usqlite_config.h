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

#ifndef usqlite_config_h
#define usqlite_config_h

// ------------------------------------------------------------------------------
// usqlite configuration options

#undef USQLITE_DEBUG

// ------------------------------------------------------------------------------
// SQLite configuration options - https://sqlite.org/compile.html

#define SQLITE_OS_OTHER 1

#define SQLITE_OMIT_ANALYZE 1
// #define SQLITE_OMIT_ATTACH 1
#define SQLITE_OMIT_AUTHORIZATION 1
#define SQLITE_OMIT_AUTOINIT 1
// #define SQLITE_OMIT_DECLTYPE 1
#define SQLITE_OMIT_DEPRECATED 1
#define SQLITE_OMIT_EXPLAIN 1
#define SQLITE_OMIT_LOAD_EXTENSION 1
#define SQLITE_OMIT_LOCALTIME 1
#define SQLITE_OMIT_LOOKASIDE 1
#define SQLITE_OMIT_PROGRESS_CALLBACK 1
#define SQLITE_OMIT_QUICKBALANCE 1
#define SQLITE_OMIT_SHARED_CACHE 1
#define SQLITE_OMIT_TCL_VARIABLE 1
#define SQLITE_OMIT_UTF16 1
// #define SQLITE_OMIT_WAL 1

#define SQLITE_UNTESTABLE 1

#define SQLITE_DEFAULT_SYNCHRONOUS 2
#define SQLITE_DEFAULT_WAL_SYNCHRONOUS 1
#define SQLITE_THREADSAFE 0
#define SQLITE_LIKE_DOESNT_MATCH_BLOBS 1
#define SQLITE_MAX_EXPR_DEPTH 0
#define SQLITE_DEFAULT_LOCKING_MODE 1
#undef SQLITE_ENABLE_RTREE

#define SQLITE_ENABLE_MEMORY_MANAGEMENT 1
#define SQLITE_DEFAULT_MEMSTATUS 0
#define SQLITE_ZERO_MALLOC 1


// ------------------------------------------------------------------------------

// Give SQLite one dedicated heap (MEMSYS5) instead of a GC-heap allocation per
// call. The per-call allocator (gc_alloc) puts SQLite's structures on the
// MicroPython GC heap, where the conservative collector cannot follow SQLite's
// interior/tagged pointers and frees live memory on any gc.collect() under an
// open connection -- corrupting the database or hanging the board. One pooled
// block is opaque to the GC and fixes both. See usqlite_mem.c.
#ifdef SQLITE_ZERO_MALLOC
#define SQLITE_ENABLE_MEMSYS5 1
#endif

#ifdef SQLITE_ENABLE_MEMSYS5
// Size of that pool, reserved lazily on the first connect() (see
// usqlite_mem.c), so a program that never opens a database pays nothing. Small
// by default so it fits constrained targets -- a plain RP2040 or ESP32 has only
// a few hundred KB of RAM -- and raised per board where there is room:
//   -DMEMSYS5_HEAP_SIZE=0x400000   (e.g. 4 MB on a board with PSRAM)
#ifndef MEMSYS5_HEAP_SIZE
#define MEMSYS5_HEAP_SIZE               (128 * 1024)
#endif
// Page cache defaults to half the pool (negative = KiB) so it always fits
// inside it and scales with MEMSYS5_HEAP_SIZE; override to tune.
#ifndef SQLITE_DEFAULT_CACHE_SIZE
#define SQLITE_DEFAULT_CACHE_SIZE       (-(MEMSYS5_HEAP_SIZE / 2048))
#endif
#endif

// ------------------------------------------------------------------------------

#endif
