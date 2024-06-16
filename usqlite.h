/*
MIT License

Copyright(c) 2024 Elvin Slavik

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

#ifndef usqlite_h
#define usqlite_h

#include "usqlite_config.h"

#include "sqlite3.h"

#include "usqlite_connection.h"
#include "usqlite_cursor.h"
#include "usqlite_row.h"
#include "usqlite_utils.h"
#include "usqlite_file.h"

#include <string.h>

#define _STRINGIFY(x) #x
#define STRINGIFY(x) _STRINGIFY(x)

// ------------------------------------------------------------------------------

#define USQLITE_VERSION_MAJOR   0
#define USQLITE_VERSION_MINOR   1
#define USQLITE_VERSION_MICRO   6

#define USQLITE_VERSION         STRINGIFY(USQLITE_VERSION_MAJOR) "." STRINGIFY(USQLITE_VERSION_MINOR) "." STRINGIFY(USQLITE_VERSION_MICRO)

// ------------------------------------------------------------------------------


#define ___FILELINE___ "<" __FILE__ ":" STRINGIFY(__LINE__) ">"

#ifdef _MSC_VER
#define ___FUNC___ __FUNCDNAME__ "()"
#define LOGFUNC                 usqlite_logprintf(___FUNC___ "\n")
#else
#define ___FUNC___      ___FILELINE___
#define LOGFUNC             usqlite_logprintf("%s()\n", __FUNCTION__)
#endif

#define LOGLINE                 usqlite_logprintf(___FILELINE___ "\n")


#define _UNUSED(v)      (void)v

#ifndef USQLITE_DEBUG

#undef LOGFUNC
#define LOGFUNC
#undef LOGLINE
#define LOGLINE

#endif

// ------------------------------------------------------------------------------

#endif
