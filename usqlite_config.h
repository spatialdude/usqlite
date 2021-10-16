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

//------------------------------------------------------------------------------
// usqlite configuration options

#undef USQLITE_DEBUG

//------------------------------------------------------------------------------
// SQLite configuration options - https://sqlite.org/compile.html

#define SQLITE_OS_OTHER 1

#define SQLITE_OMIT_ANALYZE 1
#define SQLITE_OMIT_ATTACH 1
#define SQLITE_OMIT_AUTHORIZATION 1
#define SQLITE_OMIT_AUTOINIT 1
//#define SQLITE_OMIT_DECLTYPE 1
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
//#define SQLITE_OMIT_WAL 1

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


//------------------------------------------------------------------------------

#ifdef SQLITE_ZERO_MALLOC
//#define SQLITE_ENABLE_MEMSYS5 1
#endif

#ifdef SQLITE_ENABLE_MEMSYS5
#define MEMSYS5_HEAP_SIZE		128 * 1024
#endif

//------------------------------------------------------------------------------

#endif
