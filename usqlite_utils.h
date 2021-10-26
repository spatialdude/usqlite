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

#ifndef usqlite_utils_h
#define usqlite_utils_h

#include "py/obj.h"

// ------------------------------------------------------------------------------

#ifdef USQLITE_DEBUG
int usqlite_logprintf(const char *fmt, ...);
int usqlite_errprintf(const char *fmt, ...);
#else
#define usqlite_logprintf(...)
#define usqlite_errprintf(...)
#endif

// ------------------------------------------------------------------------------

extern const mp_obj_type_t usqlite_Error;

void usqlite_raise_error(int rc);
void usqlite_raise(sqlite3 *db, const char *msg);
bool usqlite_lookup(mp_obj_t obj, qstr attr, mp_obj_t *dest);
mp_obj_t usqlite_method(const mp_obj_module_t *module, qstr name);

// ------------------------------------------------------------------------------

mp_obj_t usqlite_column_name(sqlite3_stmt *stmt, int column);
mp_obj_t usqlite_column_value(sqlite3_stmt *stmt, int column);
mp_obj_t usqlite_column_type(sqlite3_stmt *stmt, int column);
#ifndef SQLITE_OMIT_DECLTYPE
mp_obj_t usqlite_column_decltype(sqlite3_stmt *stmt, int column);
#endif
// ------------------------------------------------------------------------------
#endif
