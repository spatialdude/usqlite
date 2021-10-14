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
    mp_obj_list_init(&self->cursors, 0);

    return MP_OBJ_FROM_PTR(self);
}

//------------------------------------------------------------------------------

STATIC void usqlite_connection_print(const mp_print_t* print, mp_obj_t self_in, mp_print_kind_t kind) 
{
    usqlite_connection_t* self = MP_OBJ_TO_PTR(self_in);

    mp_printf(print, "<%s '%s'>", mp_obj_get_type_str(self_in), self->db ? sqlite3_db_filename(self->db, NULL) : "");
}

//------------------------------------------------------------------------------

STATIC mp_obj_t usqlite_connection_close(mp_obj_t self_in) 
{
    LOGFUNC;

    usqlite_connection_t* self = (usqlite_connection_t*)MP_OBJ_TO_PTR(self_in);
    if (!self->db)
    {
        return mp_const_none;
    }

    for (int i = 0; i < self->cursors.len; i++)
    {
        mp_obj_t cursor = self->cursors.items[i];
        self->cursors.items[0] = mp_const_none;
        usqlite_cursor_close(cursor);
#if MICROPY_MALLOC_USES_ALLOCATED_SIZE
        m_free(MP_OBJ_TO_PTR(cursor), sizeof(usqlite_cursor_t));
#else
        m_free(MP_OBJ_TO_PTR(cursor));
#endif
    }

    usqlite_logprintf(___FUNC___ " closing '%s'\n", sqlite3_db_filename(self->db, NULL));
    sqlite3_close(self->db);
    self->db = NULL;

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

    mp_obj_t args[2] =
    {
        MP_OBJ_FROM_PTR(self),
        mp_const_none
    };

    return usqlite_cursor_type.make_new(NULL, MP_ARRAY_SIZE(args), 0, args);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(usqlite_connection_cursor_obj, usqlite_connection_cursor);

//------------------------------------------------------------------------------

STATIC mp_obj_t usqlite_connection_execute(size_t n_args, const mp_obj_t* args)
{
    usqlite_connection_t* self = MP_OBJ_TO_PTR(args[0]);

    mp_obj_t xargs[4] =
    {
        MP_OBJ_FROM_PTR(self),
        mp_const_false,
        args[1]
    };

    size_t nxargs = 3;

    if (n_args == 3)
    {
        xargs[3] = args[2];
        nxargs++;
    }

    return usqlite_cursor_type.make_new(NULL, nxargs, 0, xargs);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(usqlite_connection_execute_obj, 2, 3, usqlite_connection_execute);

//------------------------------------------------------------------------------

STATIC mp_obj_t usqlite_connection_executemany(mp_obj_t self_in, mp_obj_t sql)
{
    usqlite_connection_t* self = MP_OBJ_TO_PTR(self_in);

    mp_obj_t args[3] =
    {
        MP_OBJ_FROM_PTR(self),
        mp_const_true,
        sql
    };

    return usqlite_cursor_type.make_new(NULL, MP_ARRAY_SIZE(args), 0, args);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_2(usqlite_connection_executemany_obj, usqlite_connection_executemany);

//------------------------------------------------------------------------------

STATIC int traceCallback(unsigned uMask, void* context, void* p, void* x)
{
    usqlite_connection_t* self = (usqlite_connection_t*)context;
    sqlite3_stmt* stmt = (sqlite3_stmt*)p;
    char* xsql = sqlite3_expanded_sql(stmt);
    if (xsql)
    {
        mp_call_function_1(self->trace_callback, mp_obj_new_str(xsql, strlen(xsql)));
        sqlite3_free(xsql);
    }
    else
    {
        const char* sql = sqlite3_sql(stmt);
        mp_call_function_1(self->trace_callback, mp_obj_new_str(sql, strlen(sql)));
    }

    return 0;
}

STATIC mp_obj_t usqlite_connection_set_trace_callback(mp_obj_t self_in, mp_obj_t callback)
{
    usqlite_connection_t* self = MP_OBJ_TO_PTR(self_in);

    if (callback == mp_const_none)
    {
        self->trace_callback = mp_const_none;
        sqlite3_trace_v2(self->db, 0, NULL, NULL);
    }
    else if (mp_obj_is_callable(callback))
    {
        self->trace_callback = callback;
        sqlite3_trace_v2(self->db, SQLITE_TRACE_STMT, traceCallback, self);
    }
    else
    {
        mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("Invalid callback"));
    }

    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_2(usqlite_connection_set_trace_callback_obj, usqlite_connection_set_trace_callback);

//------------------------------------------------------------------------------

void usqlite_connection_register(usqlite_connection_t* connection, mp_obj_t cursor)
{
    mp_obj_t cursors = MP_OBJ_FROM_PTR(&connection->cursors);
    mp_obj_list_append(cursors, cursor);
}

//------------------------------------------------------------------------------

void usqlite_connection_deregister(usqlite_connection_t* connection, mp_obj_t cursor)
{
    mp_obj_t cursors = MP_OBJ_FROM_PTR(&connection->cursors);
    mp_obj_list_remove(cursors, cursor);
}

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

        case MP_QSTR_total_changes:
            dest[0] = mp_obj_new_int(sqlite3_total_changes(self->db));
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
    { MP_ROM_QSTR(MP_QSTR___del__),             MP_ROM_PTR(&usqlite_connection_del_obj) },
    { MP_ROM_QSTR(MP_QSTR___enter__),           MP_ROM_PTR(&mp_identity_obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__),            MP_ROM_PTR(&usqlite_connection_exit_obj) },

    { MP_ROM_QSTR(MP_QSTR_close),               MP_ROM_PTR(&usqlite_connection_close_obj) },
    { MP_ROM_QSTR(MP_QSTR_cursor),              MP_ROM_PTR(&usqlite_connection_cursor_obj) },
    { MP_ROM_QSTR(MP_QSTR_execute),             MP_ROM_PTR(&usqlite_connection_execute_obj) },
    { MP_ROM_QSTR(MP_QSTR_executemany),         MP_ROM_PTR(&usqlite_connection_executemany_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_trace_callback),  MP_ROM_PTR(&usqlite_connection_set_trace_callback_obj) },
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
