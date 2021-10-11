#ifndef usqlite_h
#define usqlite_h

#include "usqlite_config.h"

#include "../../sqlite/src/sqlite3.h"

#include "usqlite_connection.h"
#include "usqlite_cursor.h"
#include "usqlite_row.h"
#include "usqlite_utils.h"
#include "usqlite_file.h"

#include <string.h>

#define _STRINGIFY(x) #x
#define STRINGIFY(x) _STRINGIFY(x)

//------------------------------------------------------------------------------

#define USQLITE_VERSION_MAJOR	0
#define USQLITE_VERSION_MINOR	1
#define USQLITE_VERSION_MICRO	0

#define USQLITE_VERSION		STRINGIFY(USQLITE_VERSION_MAJOR) "." STRINGIFY(USQLITE_VERSION_MINOR) "." STRINGIFY(USQLITE_VERSION_MICRO)

//------------------------------------------------------------------------------

#undef USQLITE_DEBUGLOG

//------------------------------------------------------------------------------


#define ___FILELINE___ "<" __FILE__ ":" STRINGIFY(__LINE__) ">"

#ifdef _MSC_VER
#define ___FUNC___ __FUNCDNAME__ "()"
#define LOGFUNC			usqlite_logprintf(___FUNC___ "\n")
#else
#define ___FUNC___	___FILELINE___
#define LOGFUNC		    usqlite_logprintf("%s()\n", __FUNCTION__)
#endif

#define LOGLINE			usqlite_logprintf(___FILELINE___ "\n")


#define _UNUSED(v)	(void)v

#ifndef USQLITE_DEBUGLOG

#undef LOGFUNC
#define LOGFUNC
#undef LOGLINE
#define LOGLINE

#endif

//------------------------------------------------------------------------------

#endif
