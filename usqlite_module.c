#include "py/runtime.h"
#include "py/objstr.h"
#include "py/stream.h"
#include "py/reader.h"

#include "string.h"

#include "usqlite.h"
#include "usqlite_mem.h"

//------------------------------------------------------------------------------

STATIC const MP_DEFINE_STR_OBJ(usqlite_version, USQLITE_VERSION);

STATIC const mp_rom_obj_tuple_t usqlite_version_info = {
    { &mp_type_tuple },
    3,
    {
        MP_ROM_INT(USQLITE_VERSION_MAJOR),
        MP_ROM_INT(USQLITE_VERSION_MINOR),
        MP_ROM_INT(USQLITE_VERSION_MICRO)
    },
};


STATIC const MP_DEFINE_STR_OBJ(sqlite_version, SQLITE_VERSION);

#define SQLITE_VERSION_MAJOR  (int)(SQLITE_VERSION_NUMBER/1000000)
#define SQLITE_VERSION_MINOR  (int)((SQLITE_VERSION_NUMBER - SQLITE_VERSION_MAJOR*1000000)/1000)
#define SQLITE_VERSION_MICRO  (SQLITE_VERSION_NUMBER - SQLITE_VERSION_MAJOR*1000000 - SQLITE_VERSION_MINOR*1000)

STATIC const mp_rom_obj_tuple_t sqlite_version_info = {
    { &mp_type_tuple },
    3,
    {
        MP_ROM_INT(SQLITE_VERSION_MAJOR),
        MP_ROM_INT(SQLITE_VERSION_MINOR),
        MP_ROM_INT(SQLITE_VERSION_MICRO)
    },
};

//------------------------------------------------------------------------------

STATIC void initialize()
{
    static int initialized = 0;

    if (initialized)
    {
        return;
    }

    usqlite_mem_init();

    int rc = sqlite3_initialize();
    if (rc)
    {
        usqlite_raise_error(rc);
        return;
    }

    initialized = 1;
}

//------------------------------------------------------------------------------

STATIC mp_obj_t usqlite_init(void)
{
    LOGFUNC;

    //initialize();

    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(usqlite_init_obj, usqlite_init);

//------------------------------------------------------------------------------

STATIC mp_obj_t usqlite_connect(mp_obj_t filename) 
{
    initialize();

    const char* pFilename = mp_obj_str_get_str(filename);

    if (!pFilename || !strlen(pFilename))
    {
        mp_raise_msg(&usqlite_Error, MP_ERROR_TEXT("Empty filename"));
        return mp_const_none;
    }

    usqlite_logprintf(___FUNC___ " filename: '%s'\n", pFilename);

    sqlite3* db = NULL;
    int rc = sqlite3_open(pFilename, &db);
    if (rc)
    {
        mp_raise_msg_varg(&usqlite_Error, MP_ERROR_TEXT("error %d:%s opening '%s'"), rc, sqlite3_errstr(rc), pFilename);
        return mp_const_none;
    }

    usqlite_logprintf(___FUNC___ " connected: '%s'\n", pFilename);

    mp_obj_t args[1] =
    {
        MP_OBJ_FROM_PTR(db)
    };

    return usqlite_connection_type.make_new(NULL, 1, 0, args);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(usqlite_connect_obj, usqlite_connect);

//------------------------------------------------------------------------------

STATIC mp_obj_t usqlite_mem_peak(size_t n_args, const mp_obj_t* args)
{
    initialize();

    int reset = 0;
    if (n_args > 0)
    {
        reset = mp_obj_get_int(args[0]);
    }

    sqlite3_int64 mem = sqlite3_memory_highwater(reset != 0);

    return mp_obj_new_int_from_ll(mem);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(usqlite_mem_peak_obj, 0, 1, usqlite_mem_peak);

//------------------------------------------------------------------------------

STATIC mp_obj_t usqlite_mem_status(size_t n_args, const mp_obj_t* args)
{
    static int mem_status = SQLITE_DEFAULT_MEMSTATUS;

    if (n_args > 0)
    {
        mem_status = mp_obj_get_int(args[0]);
        int rc = sqlite3_config(SQLITE_CONFIG_MEMSTATUS, mem_status);
        if (rc)
        {
            usqlite_raise_error(rc);
        }
    }

    return mp_obj_new_bool(mem_status);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(usqlite_mem_status_obj, 0, 1, usqlite_mem_status);

//------------------------------------------------------------------------------

STATIC mp_obj_t usqlite_mem_current(void)
{
    initialize();

    sqlite3_int64 mem = sqlite3_memory_used();

    return mp_obj_new_int_from_ll(mem);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(usqlite_mem_current_obj, usqlite_mem_current);

//------------------------------------------------------------------------------

STATIC const mp_rom_map_elem_t usqlite_module_globals_table[] =
{
    { MP_ROM_QSTR(MP_QSTR___name__),                MP_ROM_QSTR(MP_QSTR_usqlite) },
    { MP_ROM_QSTR(MP_QSTR___init__),                MP_ROM_PTR(&usqlite_init_obj) },

    { MP_ROM_QSTR(MP_QSTR_version),                 MP_ROM_PTR(&usqlite_version) },
    { MP_ROM_QSTR(MP_QSTR_version_info),            MP_ROM_PTR(&usqlite_version_info) },

    { MP_ROM_QSTR(MP_QSTR_sqlite_version),          MP_ROM_PTR(&sqlite_version) },
    { MP_ROM_QSTR(MP_QSTR_sqlite_version_info),     MP_ROM_PTR(&sqlite_version_info) },
    { MP_ROM_QSTR(MP_QSTR_sqlite_version_number),   MP_ROM_INT(SQLITE_VERSION_NUMBER) },

    { MP_ROM_QSTR(MP_QSTR_connect),                 MP_ROM_PTR(&usqlite_connect_obj) },

    { MP_ROM_QSTR(MP_QSTR_mem_peak),                MP_ROM_PTR(&usqlite_mem_peak_obj) },
    { MP_ROM_QSTR(MP_QSTR_mem_current),             MP_ROM_PTR(&usqlite_mem_current_obj) },
    { MP_ROM_QSTR(MP_QSTR_mem_status),              MP_ROM_PTR(&usqlite_mem_status_obj) },
};

STATIC MP_DEFINE_CONST_DICT(usqlite_module_globals, usqlite_module_globals_table);

//------------------------------------------------------------------------------

const mp_obj_module_t usqlite_module =
{
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&usqlite_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_usqlite, usqlite_module, 1);

//------------------------------------------------------------------------------
