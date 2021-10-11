#ifndef usqlite_connection_h
#define usqlite_connection_h

#include "py/runtime.h"
#include "py/obj.h"

//------------------------------------------------------------------------------

typedef struct _usqlite_connection_t 
{
    mp_obj_base_t   base;
    sqlite3*        db;
    qstr            row_type;
    mp_obj_t        row_factory;
} 
usqlite_connection_t;

//------------------------------------------------------------------------------

extern const mp_obj_type_t usqlite_connection_type;

//------------------------------------------------------------------------------

#endif
