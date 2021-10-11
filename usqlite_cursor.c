#include "usqlite.h"

#include "py/objstr.h"
#include "py/objtuple.h"

//------------------------------------------------------------------------------

STATIC mp_obj_t usqlite_cursor_execute(mp_obj_t self_in, mp_obj_t sql_in);
STATIC mp_obj_t usqlite_cursor_executemany(mp_obj_t self_in, mp_obj_t sql_in);

STATIC mp_obj_t row_tuple(usqlite_cursor_t* cursor);
STATIC mp_obj_t row_dict(usqlite_cursor_t* cursor);
STATIC mp_obj_t row_type(usqlite_cursor_t* cursor);

//------------------------------------------------------------------------------

STATIC mp_obj_t usqlite_cursor_make_new(const mp_obj_type_t* type, size_t n_args, size_t n_kw, const mp_obj_t* args)
{
    usqlite_row_type_initialize();

    usqlite_cursor_t* self = m_new_obj(usqlite_cursor_t);
    mp_obj_t self_obj = MP_OBJ_FROM_PTR(self);

    memset(self, 0, sizeof(usqlite_cursor_t));

    self->base.type = &usqlite_cursor_type;
    self->connection = (usqlite_connection_t*)MP_OBJ_TO_PTR(args[0]);

    usqlite_connection_register(self->connection, self_obj);

    switch (self->connection->row_type)
    {
    case MP_QSTR_row:
        self->rowfactory = row_type;
        break;

    case MP_QSTR_dict:
        self->rowfactory = row_dict;
        break;

    case MP_QSTR_tuple:
    default:
        self->rowfactory = row_tuple;
        break;
    }

    if (n_args > 2)
    {
        return usqlite_cursor_executemany(self_obj, args[1]);
    }
    else if (n_args > 1)
    {
        return usqlite_cursor_execute(self_obj, args[1]);
    }

    return self_obj;
}

//------------------------------------------------------------------------------

STATIC void usqlite_cursor_print(const mp_print_t* print, mp_obj_t self_in, mp_print_kind_t kind) 
{
    usqlite_cursor_t* self = MP_OBJ_TO_PTR(self_in);

    mp_printf(print, "<%s '%s'>", mp_obj_get_type_str(self_in), self->stmt ? sqlite3_sql(self->stmt) : "NULL");
}

//------------------------------------------------------------------------------

mp_obj_t usqlite_cursor_close(mp_obj_t self_in)
{
    LOGFUNC;
    //usqlite_logprintf(___FUNC___ "\n");

    usqlite_cursor_t* self = (usqlite_cursor_t*)MP_OBJ_TO_PTR(self_in);
    if (!self->stmt)
    {
        return mp_const_none;
    }

    usqlite_logprintf(___FUNC___ " closing: '%s'\n", sqlite3_sql(self->stmt));
    sqlite3_finalize(self->stmt);
    self->stmt = NULL;
    self->rowcount = -1;
    self->rc = SQLITE_OK;

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(usqlite_cursor_close_obj, usqlite_cursor_close);

//------------------------------------------------------------------------------

STATIC int step(usqlite_cursor_t* self)
{
    self->rc = sqlite3_step(self->stmt);

    switch (self->rc)
    {
    case SQLITE_OK:
        break;

    case SQLITE_ROW:
        self->rowcount++;
        break;

    case SQLITE_DONE:
        break;

    case SQLITE_ERROR:
    default:
        mp_raise_msg_varg(&usqlite_Error, 
            MP_ERROR_TEXT("error (%d): %s sql: '%s'"), 
            self->rc,
            sqlite3_errmsg(self->connection->db),
            sqlite3_sql(self->stmt));
        break;
    }

    return self->rc;
}

//------------------------------------------------------------------------------

STATIC mp_obj_t usqlite_cursor_execute(mp_obj_t self_in, mp_obj_t sql_in)
{
    usqlite_cursor_t* self = MP_OBJ_TO_PTR(self_in);
    const char* sql = mp_obj_str_get_str(sql_in);

    usqlite_cursor_close(self_in);

    if (!sql || !*sql)
    {
        mp_raise_msg(&usqlite_Error, MP_ERROR_TEXT("Empty sql"));
        return mp_const_none;
    }

    int result = sqlite3_prepare_v2(self->connection->db, sql, strlen(sql), &self->stmt, NULL);
    if (result)
    {
        mp_raise_msg_varg(&usqlite_Error, 
            MP_ERROR_TEXT("sqlite3 error: (%d) %s preparing '%s'"), 
            result,
            sqlite3_errmsg(self->connection->db),
            sql);
        sqlite3_finalize(self->stmt);
        self->stmt = NULL;
        return mp_const_none;
    }

    self->rowcount = 0;

    step(self);

    switch (self->rc)
    {
    case SQLITE_ROW:
        break;

    case SQLITE_DONE:
        self->rowcount = sqlite3_changes(self->connection->db);
        break;

    default:
        self->rowcount = -1;
        break;
    }

    return self_in;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_2(usqlite_cursor_execute_obj, usqlite_cursor_execute);

//------------------------------------------------------------------------------

STATIC mp_obj_t usqlite_cursor_executemany(mp_obj_t self_in, mp_obj_t sql_in)
{
    usqlite_cursor_t* self = MP_OBJ_TO_PTR(self_in);
    const char* sql = mp_obj_str_get_str(sql_in);

    usqlite_cursor_close(self_in);

    if (!sql || !*sql)
    {
        mp_raise_msg(&usqlite_Error, MP_ERROR_TEXT("Empty sql"));
        return mp_const_none;
    }

    char* errmsg = NULL;

    int rc = sqlite3_exec(self->connection->db, sql, NULL, NULL, &errmsg);
    if (rc)
    {
        mp_raise_msg_varg(&usqlite_Error, MP_ERROR_TEXT("%s"), errmsg ? errmsg : "");
    }
    
    sqlite3_free(errmsg);

    return self_in;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_2(usqlite_cursor_executemany_obj, usqlite_cursor_executemany);

//------------------------------------------------------------------------------

STATIC mp_obj_t usqlite_cursor_getiter(mp_obj_t self_in, mp_obj_iter_buf_t* iter_buf) 
{
    usqlite_cursor_t* self = MP_OBJ_TO_PTR(self_in);
    (void)iter_buf;

    if (!self->stmt)
    {
        mp_raise_msg(&usqlite_Error, MP_ERROR_TEXT("No iter data"));
        return mp_const_none;
    }

    return self;
}

//------------------------------------------------------------------------------

STATIC mp_obj_t row_dict(usqlite_cursor_t* cursor)
{
    int columns = sqlite3_data_count(cursor->stmt);

    mp_obj_t dict = mp_obj_new_dict(columns);

    for (int i = 0; i < columns; i++)
    {
        mp_obj_dict_store(dict, usqlite_column_name(cursor->stmt, i), usqlite_column_value(cursor->stmt, i));
    }

    return dict;
}

//------------------------------------------------------------------------------

STATIC mp_obj_t row_tuple(usqlite_cursor_t* cursor)
{
    int columns = sqlite3_data_count(cursor->stmt);

    mp_obj_tuple_t* o = m_new_obj_var(mp_obj_tuple_t, mp_obj_t, columns);
    o->base.type = &mp_type_tuple;
    o->len = columns;

    for (int i = 0; i < columns; i++)
    {
        o->items[i] = usqlite_column_value(cursor->stmt, i);
    }

    return MP_OBJ_FROM_PTR(o);
}

//------------------------------------------------------------------------------

STATIC mp_obj_t row_type(usqlite_cursor_t* cursor)
{
    int columns = sqlite3_data_count(cursor->stmt);

    mp_obj_tuple_t* o = m_new_obj_var(mp_obj_tuple_t, mp_obj_t, columns + 1);
    o->base.type = &usqlite_row_type;
    o->len = columns;

    o->items[columns] = MP_OBJ_FROM_PTR(cursor);

    for (int i = 0; i < columns; i++)
    {
        o->items[i] = usqlite_column_value(cursor->stmt, i);
    }

    return MP_OBJ_FROM_PTR(o);
}

//------------------------------------------------------------------------------

STATIC mp_obj_t usqlite_cursor_iternext(mp_obj_t self_in)
{
    usqlite_cursor_t* self = MP_OBJ_TO_PTR(self_in);
    mp_obj_t result = self->rc == SQLITE_ROW
        ? self->rowfactory(self)
        : MP_OBJ_STOP_ITERATION;


    switch (self->rc)
    {
    case SQLITE_OK:
        break;

    case SQLITE_ROW:
        step(self);
        break;

    case SQLITE_DONE:
        //self->rc = sqlite3_reset(self->stmt);
        break;

    case SQLITE_ERROR:
    default:
        mp_raise_msg_varg(&mp_type_RuntimeError, MP_ERROR_TEXT("sqlite3 error %d executing '%s'"), self->rc, sqlite3_sql(self->stmt));
        break;
    }

    return result;
}

//------------------------------------------------------------------------------

STATIC mp_obj_t usqlite_cursor_description(sqlite3_stmt* stmt)
{
    int columns = sqlite3_data_count(stmt);

    mp_obj_tuple_t* o = m_new_obj_var(mp_obj_tuple_t, mp_obj_t, columns);
    o->base.type = &mp_type_tuple;
    o->len = columns;

    for (int i = 0; i < columns; i++)
    {
        mp_obj_tuple_t* c = m_new_obj_var(mp_obj_tuple_t, mp_obj_t, 7);
        c->base.type = &mp_type_tuple;
        c->len = 7;

        c->items[0] = usqlite_column_name(stmt, i);
        c->items[1] = usqlite_column_type(stmt, i);
        for (int j = 2; j < 7; j++)
        {
            c->items[j] = mp_const_none;
        }

        o->items[i] = MP_OBJ_FROM_PTR(c);
    }

    return MP_OBJ_FROM_PTR(o);
}

//------------------------------------------------------------------------------

STATIC void usqlite_cursor_attr(mp_obj_t self_in, qstr attr, mp_obj_t* dest)
{
    usqlite_cursor_t* self = MP_OBJ_TO_PTR(self_in);

    if (dest[0] == MP_OBJ_NULL)
    {
        if ((usqlite_lookup(self_in, attr, dest)))
        {
            return;
        }

        switch (attr)
        {
        case MP_QSTR_description:
            dest[0] = usqlite_cursor_description(self->stmt);
            break;

        case MP_QSTR_rowcount:
            dest[0] = mp_obj_new_int(self->rowcount);
            break;
        }
    }
    else if (dest[1] != MP_OBJ_NULL)
    {
        //dest[0] = MP_OBJ_NULL; // indicate success
    }
}

//------------------------------------------------------------------------------

STATIC mp_obj_t usqlite_cursor_del(mp_obj_t self_in)
{
    usqlite_cursor_t* self = MP_OBJ_TO_PTR(self_in);

    usqlite_logprintf(___FUNC___ "\n");

    usqlite_cursor_close(self_in);
    usqlite_connection_deregister(self->connection, self_in);

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(usqlite_cursor_del_obj, usqlite_cursor_del);

//------------------------------------------------------------------------------

STATIC mp_obj_t usqlite_cursor_exit(size_t n_args, const mp_obj_t* args)
{
    usqlite_logprintf(___FUNC___ "\n");

    usqlite_cursor_close(args[0]);

    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(usqlite_cursor_exit_obj, 4, 4, usqlite_cursor_exit);

//------------------------------------------------------------------------------

STATIC const mp_rom_map_elem_t usqlite_cursor_locals_dict_table[] = 
{
    { MP_ROM_QSTR(MP_QSTR___del__),     MP_ROM_PTR(&usqlite_cursor_del_obj) },
    { MP_ROM_QSTR(MP_QSTR___enter__),   MP_ROM_PTR(&mp_identity_obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__),    MP_ROM_PTR(&usqlite_cursor_exit_obj) },

    { MP_ROM_QSTR(MP_QSTR_close),       MP_ROM_PTR(&usqlite_cursor_close_obj) },
    { MP_ROM_QSTR(MP_QSTR_execute),     MP_ROM_PTR(&usqlite_cursor_execute_obj) },
    { MP_ROM_QSTR(MP_QSTR_executemany), MP_ROM_PTR(&usqlite_cursor_executemany_obj) },
};

MP_DEFINE_CONST_DICT(usqlite_cursor_locals_dict, usqlite_cursor_locals_dict_table);

//------------------------------------------------------------------------------

const mp_obj_type_t usqlite_cursor_type = 
{
    { &mp_type_type },
    .name = MP_QSTR_Cursor,
    .print = usqlite_cursor_print,
    .make_new = usqlite_cursor_make_new,
    .getiter = usqlite_cursor_getiter,
    .iternext = usqlite_cursor_iternext,
    .locals_dict = (mp_obj_dict_t*)&usqlite_cursor_locals_dict,
    .attr = &usqlite_cursor_attr
};

//------------------------------------------------------------------------------
