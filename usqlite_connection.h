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

#ifndef usqlite_connection_h
#define usqlite_connection_h

#include "py/runtime.h"
#include "py/obj.h"

// ------------------------------------------------------------------------------

typedef struct _usqlite_connection_t
{
    mp_obj_base_t base;
    sqlite3 *db;
    qstr row_type;
    mp_obj_t row_factory;
    mp_obj_list_t cursors;
    mp_obj_t trace_callback;
}
usqlite_connection_t;

// ------------------------------------------------------------------------------

extern const mp_obj_type_t usqlite_connection_type;

void usqlite_connection_register(usqlite_connection_t *connection, mp_obj_t cursor);
void usqlite_connection_deregister(usqlite_connection_t *connection, mp_obj_t cursor);

// ------------------------------------------------------------------------------

#endif
