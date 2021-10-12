#ifndef usqlite_config_h
#define usqlite_config_h

//------------------------------------------------------------------------------

#define SQLITE_OS_OTHER 1

#define SQLITE_OMIT_ANALYZE 1
#define SQLITE_OMIT_ATTACH 1
#define SQLITE_OMIT_AUTHORIZATION 1
#define SQLITE_OMIT_AUTOINIT 1
#define SQLITE_OMIT_DECLTYPE 1
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
#define SQLITE_DEFAULT_MEMSTATUS 1
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
