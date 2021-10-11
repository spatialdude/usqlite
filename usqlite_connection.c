#include "usqlite.h"

#include "py/objstr.h"

//------------------------------------------------------------------------------

STATIC mp_obj_t usqlite_connection_close(mp_obj_t self_in);

//------------------------------------------------------------------------------

STATIC mp_obj_t usqlite_connection_make_new(const mp_obj_type_t* type, size_t n_args, size_t n_kw, const mp_obj_t* args)
{
    usqlite_connection_t* self = m_new_obj(usqlite_connection_t);

    self->base.type = &usqlite_connection_type;
    self->db = (sqlite3*)MP_OBJ_TO_PTR(args[0]);
    self->row_type = MP_QSTR_tuple;

    return MP_OBJ_FROM_PTR(self);
}

//------------------------------------------------------------------------------

STATIC void usqlite_connection_print(const mp_print_t* print, mp_obj_t self_in, mp_print_kind_t kind) 
{
    usqlite_connection_t* self = MP_OBJ_TO_PTR(self_in);

    mp_printf(print, "<%s '%s'>", mp_obj_get_type_str(self_in), sqlite3_db_filename(self->db, NULL));
}

//------------------------------------------------------------------------------

STATIC mp_obj_t usqlite_connection_close(mp_obj_t self_in) 
{
    usqlite_connection_t* self = (usqlite_connection_t*)MP_OBJ_TO_PTR(self_in);

    //usqlite_logprintf(___FUNC___ "\n");

    if (self->db)
    {
        usqlite_logprintf(___FUNC___ " closing '%s'\n", sqlite3_db_filename(self->db, NULL));
        sqlite3_close(self->db);
        self->db = NULL;
    }

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(usqlite_connection_close_obj, usqlite_connection_close);

//------------------------------------------------------------------------------

STATIC mp_obj_t usqlite_connection_del(mp_obj_t self_in)
{
    usqlite_logprintf(___FUNC___, "\n");

    usqlite_connection_close(self_in);

    return mp_const_none;
};

MP_DEFINE_CONST_FUN_OBJ_1(usqlite_connection_del_obj, usqlite_connection_del);

//------------------------------------------------------------------------------

STATIC mp_obj_t usqlite_connection_cursor(mp_obj_t self_in)
{
    usqlite_connection_t* self = MP_OBJ_TO_PTR(self_in);

    mp_obj_t args[1] =
    {
        MP_OBJ_FROM_PTR(self)
    };

    return usqlite_cursor_type.make_new(NULL, MP_ARRAY_SIZE(args), 0, args);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(usqlite_connection_cursor_obj, usqlite_connection_cursor);

//------------------------------------------------------------------------------

STATIC mp_obj_t usqlite_connection_execute(mp_obj_t self_in, mp_obj_t sql)
{
    usqlite_connection_t* self = MP_OBJ_TO_PTR(self_in);

    mp_obj_t args[2] =
    {
        MP_OBJ_FROM_PTR(self),
        sql
    };

    return usqlite_cursor_type.make_new(NULL, MP_ARRAY_SIZE(args), 0, args);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_2(usqlite_connection_execute_obj, usqlite_connection_execute);

//------------------------------------------------------------------------------

STATIC mp_obj_t usqlite_connection_executemany(mp_obj_t self_in, mp_obj_t sql)
{
    usqlite_connection_t* self = MP_OBJ_TO_PTR(self_in);

    mp_obj_t args[3] =
    {
        MP_OBJ_FROM_PTR(self),
        sql,
        mp_const_true
    };

    return usqlite_cursor_type.make_new(NULL, MP_ARRAY_SIZE(args), 0, args);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_2(usqlite_connection_executemany_obj, usqlite_connection_executemany);

//------------------------------------------------------------------------------

STATIC void usqlite_connection_attr(mp_obj_t self_in, qstr attr, mp_obj_t* dest)
{
    usqlite_connection_t* self = (usqlite_connection_t*)self_in;

    if (dest[0] == MP_OBJ_NULL)
    {
        if ((usqlite_lookup(self_in, attr, dest)))
        {
            return;
        }

        switch (attr)
        {
        case MP_QSTR_row_type:
            dest[0] = MP_OBJ_NEW_QSTR(self->row_type);
            break;
        }
    }
    else if (dest[1] != MP_OBJ_NULL)
    {
        switch (attr)
        {
        case MP_QSTR_row_type:
            self->row_type = mp_obj_str_get_qstr(dest[1]);
            dest[0] = MP_OBJ_NULL;
            break;
        }

        // delete/store attribute
    }
}

//------------------------------------------------------------------------------

STATIC mp_obj_t usqlite_connection_exit(size_t n_args, const mp_obj_t* args)
{
    usqlite_logprintf(___FUNC___ "\n");

    usqlite_connection_close(args[0]);

    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(usqlite_connection_exit_obj, 4, 4, usqlite_connection_exit);

//------------------------------------------------------------------------------

STATIC const mp_rom_map_elem_t usqlite_connection_locals_dict_table[] = 
{
    { MP_ROM_QSTR(MP_QSTR___del__),     MP_ROM_PTR(&usqlite_connection_del_obj) },
    { MP_ROM_QSTR(MP_QSTR___enter__),   MP_ROM_PTR(&mp_identity_obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__),    MP_ROM_PTR(&usqlite_connection_exit_obj) },

    { MP_ROM_QSTR(MP_QSTR_close),       MP_ROM_PTR(&usqlite_connection_close_obj) },
    { MP_ROM_QSTR(MP_QSTR_cursor),      MP_ROM_PTR(&usqlite_connection_cursor_obj) },
    { MP_ROM_QSTR(MP_QSTR_execute),     MP_ROM_PTR(&usqlite_connection_execute_obj) },
    { MP_ROM_QSTR(MP_QSTR_executemany), MP_ROM_PTR(&usqlite_connection_executemany_obj) },
};

MP_DEFINE_CONST_DICT(usqlite_connection_locals_dict, usqlite_connection_locals_dict_table);

//------------------------------------------------------------------------------

const mp_obj_type_t usqlite_connection_type = 
{
    { &mp_type_type },
    .name = MP_QSTR_Connection,
    .print = usqlite_connection_print,
    .make_new = usqlite_connection_make_new,
    .locals_dict = (mp_obj_dict_t*)&usqlite_connection_locals_dict,
    .attr = usqlite_connection_attr,
};

//------------------------------------------------------------------------------
