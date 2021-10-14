//------------------------------------------------------------------------------

#include "usqlite.h"

#include <stdarg.h>

#include "py/runtime.h"
#include "py/builtin.h"
#include "py/objexcept.h"

#include "string.h"

//------------------------------------------------------------------------------

//extern const struct _mp_print_t mp_stderr_print;

//------------------------------------------------------------------------------

int usqlite_logprintf(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int ret = mp_vprintf(&mp_plat_print, fmt, ap);
    va_end(ap);
    return ret;
}

int usqlite_errprintf(const char* fmt, ...) 
{
    va_list ap;
    va_start(ap, fmt);
    int ret = mp_vprintf(&mp_plat_print, fmt, ap);
//    int ret = mp_vprintf(&mp_stderr_print, fmt, ap);
    va_end(ap);
    return ret;
}

//------------------------------------------------------------------------------

const mp_obj_type_t usqlite_Error = {
    { &mp_type_type },
    .name = MP_QSTR_usqlite_Error,
    .print = mp_obj_exception_print,
    .make_new = mp_obj_exception_make_new,
    .attr = mp_obj_exception_attr,
};

//------------------------------------------------------------------------------

void usqlite_raise_error(int rc)
{
    mp_raise_msg_varg(&usqlite_Error, MP_ERROR_TEXT("error:%s"), sqlite3_errstr(rc));
}

//------------------------------------------------------------------------------

void usqlite_raise(sqlite3* db, const char* msg)
{
    //int ec = sqlite3_errcode(db);
    //int eec = sqlite3_extended_errcode(db);
    const char* errmsg = sqlite3_errmsg(db);

    mp_raise_msg_varg(&usqlite_Error, MP_ERROR_TEXT("%s error:%s"), msg ? msg : "", errmsg);
}

//------------------------------------------------------------------------------

bool usqlite_lookup(mp_obj_t obj, qstr attr, mp_obj_t* dest)
{
    const mp_obj_type_t* type = mp_obj_get_type(obj);

#if MICROPY_CPYTHON_COMPAT
    if (attr == MP_QSTR___class__) 
    {
        // a.__class__ is equivalent to type(a)
        dest[0] = MP_OBJ_FROM_PTR(type);
        return true;
    }
#endif

    if (attr == MP_QSTR___next__ && type->iternext != NULL) 
    {
        dest[0] = MP_OBJ_FROM_PTR(&mp_builtin_next_obj);
        dest[1] = obj;
        return true;
    }

    if (type->locals_dict != NULL) 
    {
        mp_map_t* locals_map = &type->locals_dict->map;
        mp_map_elem_t* elem = mp_map_lookup(locals_map, MP_OBJ_NEW_QSTR(attr), MP_MAP_LOOKUP);
        if (elem != NULL) 
        {
            mp_convert_member_lookup(obj, type, elem->value, dest);
            return true;
        }
    }

    return false;
}

//------------------------------------------------------------------------------

mp_obj_t usqlite_column_name(sqlite3_stmt* stmt, int column)
{
    const char* name = sqlite3_column_name(stmt, column);
    return mp_obj_new_str(name, strlen(name));
}

//------------------------------------------------------------------------------

mp_obj_t usqlite_column_value(sqlite3_stmt* stmt, int column)
{
    int type = sqlite3_column_type(stmt, column);

    switch (type)
    {
    case SQLITE_NULL:
        return mp_const_none;

    case SQLITE_INTEGER:
        return mp_obj_new_int(sqlite3_column_int(stmt, column));

    case SQLITE_FLOAT:
        return mp_obj_new_float(sqlite3_column_double(stmt, column));

    case SQLITE_TEXT:
        return mp_obj_new_str((const char*)sqlite3_column_text(stmt, column), sqlite3_column_bytes(stmt, column));

    case SQLITE_BLOB:
        return mp_obj_new_bytes(sqlite3_column_blob(stmt, column), sqlite3_column_bytes(stmt, column));

    default:
        return MP_OBJ_NULL;
    }
}

//------------------------------------------------------------------------------

mp_obj_t usqlite_column_type(sqlite3_stmt* stmt, int column)
{
    int type = sqlite3_column_type(stmt, column);

    switch (type)
    {
    case SQLITE_NULL:
        return MP_OBJ_FROM_PTR(&mp_type_NoneType);

    case SQLITE_INTEGER:
        return MP_OBJ_FROM_PTR(&mp_type_int);

    case SQLITE_FLOAT:
        return MP_OBJ_FROM_PTR(&mp_type_float);

    case SQLITE_TEXT:
        return MP_OBJ_FROM_PTR(&mp_type_str);

    case SQLITE_BLOB:
        return MP_OBJ_FROM_PTR(&mp_type_bytes);

    default:
        return MP_OBJ_NULL;
    }
}

//------------------------------------------------------------------------------
#ifndef SQLITE_OMIT_DECLTYPE
mp_obj_t usqlite_column_decltype(sqlite3_stmt* stmt, int column)
{
    const char* type = sqlite3_column_decltype(stmt, column);

    return mp_obj_new_str(type, strlen(type));
}
#endif
//------------------------------------------------------------------------------

mp_obj_t usqlite_method(const mp_obj_module_t* module, qstr name)
{
    mp_obj_t dest[2];

    mp_load_method((mp_obj_t)module, name, dest);

    return dest[0];
}

//------------------------------------------------------------------------------
