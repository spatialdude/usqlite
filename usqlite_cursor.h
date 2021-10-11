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
}
usqlite_cursor_t;

//------------------------------------------------------------------------------

extern const mp_obj_type_t usqlite_cursor_type;

//------------------------------------------------------------------------------

#endif
