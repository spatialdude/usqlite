#include "py/runtime.h"
#include "py/objstr.h"
#include "py/stream.h"
#include "py/reader.h"

#include "string.h"

#include "usqlite_config.h"
#include "usqlite_utils.h"
#include "usqlite_connection.h"

//------------------------------------------------------------------------------

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

STATIC mp_obj_t usqlite_init(void)
{
    usqlite_logprintf("usqlite_init\n");

#ifdef SQLITE_OMIT_AUTOINIT
    int rc = sqlite3_initialize();
    if (rc)
    {
        mp_raise_msg(&usqlite_Error, "sqlite3_initialize");
        return mp_const_none;
    }
#endif

    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(usqlite_init_obj, usqlite_init);

//------------------------------------------------------------------------------

STATIC mp_obj_t usqlite_connect(mp_obj_t filename) 
{
    const char* pFilename = mp_obj_str_get_str(filename);

    if (!pFilename || !strlen(pFilename))
    {
        mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("Empty filename"));
        return mp_const_none;
    }

    sqlite3* db = NULL;
    int result = sqlite3_open(pFilename, &db);
    if (result)
    {
        mp_raise_msg_varg(&usqlite_Error, MP_ERROR_TEXT("sqlite3 error %d opening '%s'"), result, pFilename);
        return mp_const_none;
    }

    mp_obj_t args[1] =
    {
        MP_OBJ_FROM_PTR(db)
    };

    return usqlite_connection_type.make_new(NULL, 1, 0, args);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(usqlite_connect_obj, usqlite_connect);

//------------------------------------------------------------------------------

STATIC const mp_rom_map_elem_t usqlite_module_globals_table[] =
{
    { MP_ROM_QSTR(MP_QSTR___name__),            MP_ROM_QSTR(MP_QSTR_sqlite) },
    { MP_ROM_QSTR(MP_QSTR___init__),            MP_ROM_PTR(&usqlite_init_obj) },

    { MP_ROM_QSTR(MP_QSTR_sqlite_version),      MP_ROM_PTR(&sqlite_version) },
    { MP_ROM_QSTR(MP_QSTR_sqlite_version_info), MP_ROM_PTR(&sqlite_version_info) },

    { MP_ROM_QSTR(MP_QSTR_connect),             MP_ROM_PTR(&usqlite_connect_obj) },
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
