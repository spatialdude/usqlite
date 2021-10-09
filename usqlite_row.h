#ifndef usqlite_row_h
#define usqlite_row_h


#include "py/runtime.h"
#include "py/obj.h"

#include "usqlite_cursor.h"

//------------------------------------------------------------------------------

extern mp_obj_type_t usqlite_row_type;

//------------------------------------------------------------------------------

void usqlite_row_type_initialize();

//------------------------------------------------------------------------------

#endif
