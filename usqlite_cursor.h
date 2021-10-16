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

#ifndef usqlite_cursor_h
#define usqlite_cursor_h

#include "py/runtime.h"
#include "py/obj.h"

#include "usqlite_connection.h"

//------------------------------------------------------------------------------

typedef struct _usqlite_cursor_t usqlite_cursor_t;

typedef mp_obj_t(*usqlite_rowfactory_t)(usqlite_cursor_t*);

typedef struct _usqlite_cursor_t 
{
    mp_obj_base_t           base;
    usqlite_connection_t*   connection;
    sqlite3_stmt*           stmt;
    int                     rc;
    int                     rowcount;
    usqlite_rowfactory_t    rowfactory;
    int                     arraysize;
}
usqlite_cursor_t;

//------------------------------------------------------------------------------

extern const mp_obj_type_t usqlite_cursor_type;

mp_obj_t usqlite_cursor_close(mp_obj_t self_in);

//------------------------------------------------------------------------------

#endif
