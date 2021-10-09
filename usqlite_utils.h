//------------------------------------------------------------------------------

#ifndef usqlite_utils_h
#define usqlite_utils_h

#include "py/obj.h"
#include "sqlite3.h"

//------------------------------------------------------------------------------

int usqlite_logprintf(const char* fmt, ...);
int usqlite_errprintf(const char* fmt, ...);

extern const mp_obj_type_t usqlite_Error;

void usqlite_raise(sqlite3* db, const char* msg);
bool usqlite_lookup(mp_obj_t obj, qstr attr, mp_obj_t* dest);
mp_obj_t usqlite_method(const mp_obj_module_t* module, qstr name);

//------------------------------------------------------------------------------

mp_obj_t usqlite_column_name(sqlite3_stmt* stmt, int column);
mp_obj_t usqlite_column_value(sqlite3_stmt* stmt, int column);
mp_obj_t usqlite_column_type(sqlite3_stmt* stmt, int column);

//------------------------------------------------------------------------------
#endif