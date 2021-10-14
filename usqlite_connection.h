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
    mp_obj_list_t   cursors;
    mp_obj_t        trace_callback;
} 
usqlite_connection_t;

//------------------------------------------------------------------------------

extern const mp_obj_type_t usqlite_connection_type;

void usqlite_connection_register(usqlite_connection_t* connection, mp_obj_t cursor);
void usqlite_connection_deregister(usqlite_connection_t* connection, mp_obj_t cursor);

//------------------------------------------------------------------------------

#endif
