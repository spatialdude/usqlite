/*
MIT License

Copyright(c) 2024 Elvin Slavik

Permission is hereby granted, free of charge, to any person obtaining a copy
of this softwareand associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright noticeand this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "usqlite.h"

#include "py/objstr.h"
#include "py/objtuple.h"

#include <stdlib.h>

// ------------------------------------------------------------------------------

static mp_obj_t usqlite_cursor_execute(size_t n_args, const mp_obj_t* args);
static mp_obj_t usqlite_cursor_executemany(mp_obj_t self_in, mp_obj_t sql_in);

static mp_obj_t row_tuple(usqlite_cursor_t* cursor);
static mp_obj_t row_dict(usqlite_cursor_t* cursor);
static mp_obj_t row_type(usqlite_cursor_t* cursor);

// ------------------------------------------------------------------------------

static mp_obj_t usqlite_cursor_make_new(const mp_obj_type_t* type, size_t n_args, size_t n_kw, const mp_obj_t* args)
{
    usqlite_row_type_initialize();

    usqlite_cursor_t* self = m_new_obj(usqlite_cursor_t);
    mp_obj_t self_obj = MP_OBJ_FROM_PTR(self);

    memset(self, 0, sizeof(usqlite_cursor_t));

    self->base.type = &usqlite_cursor_type;
    self->connection = (usqlite_connection_t*)MP_OBJ_TO_PTR(args[0]);
    self->arraysize = 1;

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

    if (args[1] == mp_const_true)
    {
        return usqlite_cursor_executemany(self_obj, args[2]);
    }
    else if (args[1] == mp_const_false)
    {
        mp_obj_t xargs[3] =
        {
            self_obj,
            args[2] };
        size_t nxargs = 2;

        if (n_args == 4)
        {
            xargs[2] = args[3];
            nxargs++;
        }

        return usqlite_cursor_execute(nxargs, xargs);
    }

    return self_obj;
}

// ------------------------------------------------------------------------------

static void usqlite_cursor_print(const mp_print_t* print, mp_obj_t self_in, mp_print_kind_t kind)
{
    usqlite_cursor_t* self = MP_OBJ_TO_PTR(self_in);

    mp_printf(print, "<%s '%s'>", mp_obj_get_type_str(self_in), self->stmt ? sqlite3_sql(self->stmt) : "NULL");
}

// ------------------------------------------------------------------------------

mp_obj_t usqlite_cursor_close(mp_obj_t self_in)
{
    LOGFUNC;
    usqlite_cursor_t *self = (usqlite_cursor_t *)MP_OBJ_TO_PTR(self_in);

    // 1. Deregister FIRST to prevent memory leaks and zombies
    if (self->connection)
    {
        usqlite_connection_deregister(self->connection, self_in);
        self->connection = NULL; // Mark as detached so we don't use it again
    }

    // 2. Finalize the statement
    if (self->stmt)
    {
        usqlite_logprintf(___FUNC___ " closing: '%s'\n", sqlite3_sql(self->stmt));
        sqlite3_finalize(self->stmt);
        self->stmt = NULL;
    }
    
    self->rowcount = -1;
    self->rc = SQLITE_OK;

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(usqlite_cursor_close_obj, usqlite_cursor_close);

// ------------------------------------------------------------------------------

static int stepExecute(usqlite_cursor_t* self)
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

// ------------------------------------------------------------------------------

static int bindParameter(sqlite3_stmt* stmt, int index, mp_obj_t value)
{
    if (value == mp_const_none)
    {
        return sqlite3_bind_null(stmt, index);
    }
    else if (mp_obj_is_integer(value))
    {
        return sqlite3_bind_int(stmt, index, mp_obj_get_int(value));
    }
    else if (mp_obj_is_str(value))
    {
        GET_STR_DATA_LEN(value, str, nstr);
        // ERROR: NULL means static. changed to -1 (SQLITE_TRANSIENT) for safety.
        return sqlite3_bind_text(stmt, index, (const char*)str, nstr, (void*)-1);
    }
    else if (mp_obj_is_type(value, &mp_type_float))
    {
        return sqlite3_bind_double(stmt, index, mp_obj_get_float(value));
    }
    else if (mp_obj_is_type(value, &mp_type_bytes))
    {
        GET_STR_DATA_LEN(value, bytes, nbytes);
        // ERROR: NULL means static. changed to -1 (SQLITE_TRANSIENT) for safety.
        return sqlite3_bind_blob(stmt, index, bytes, nbytes, (void*)-1);
    }
#if MICROPY_PY_BUILTINS_BYTEARRAY
    if (mp_obj_is_type(value, &mp_type_bytearray))
    {
        mp_buffer_info_t buffer;
        if (mp_get_buffer(value, &buffer, MP_BUFFER_READ))
        {
            // ERROR: NULL means static. changed to -1 (SQLITE_TRANSIENT) for safety.
            return sqlite3_bind_blob(stmt, index, buffer.buf, buffer.len, (void*)-1);
        }
    }
#endif

    mp_raise_msg_varg(&usqlite_Error,
        MP_ERROR_TEXT("Unsupported parameter value type '%s'"),
        mp_obj_get_type_str(value));

    return -1;
}

// ------------------------------------------------------------------------------

static int bindParameters(sqlite3_stmt* stmt, mp_obj_t values)
{
    size_t nParams = sqlite3_bind_parameter_count(stmt);
    if (!nParams)
    {
        return SQLITE_OK;
    }

    const char* name = sqlite3_bind_parameter_name(stmt, 1);
    if (name && *name != '?')
    {
        if (!mp_obj_is_dict_or_ordereddict(values))
        {
            mp_raise_ValueError(MP_ERROR_TEXT("dict expected for named parameters"));
            return -1;
        }

        mp_map_t* map = mp_obj_dict_get_map(values);

        for (size_t i = 1; i <= nParams; i++)
        {
            name = sqlite3_bind_parameter_name(stmt, i);
            if (!name)
            {
                mp_raise_ValueError(MP_ERROR_TEXT("Unexpected named parameter"));
                return -1;
            }

            name++;
            mp_obj_t namestr = mp_obj_new_str(name, strlen(name));
            mp_map_elem_t* elem = mp_map_lookup(map, namestr, MP_MAP_LOOKUP);

            if (!elem)
            {
                mp_raise_msg_varg(&usqlite_Error, MP_ERROR_TEXT("Missing value for parameter '%s'"), --name);
                return -1;
            }

            int rc = bindParameter(stmt, i, elem->value);
            if (rc)
            {
                return rc;
            }
        }
    }
    else
    {
        bool namedIndex = name && *name == '?';

        size_t len = 0;
        mp_obj_t* items = NULL;

        if (mp_obj_is_type(values, &mp_type_tuple))
        {
            mp_obj_tuple_get(values, &len, &items);
        }
        else if (mp_obj_is_type(values, &mp_type_list))
        {
            mp_obj_list_get(values, &len, &items);
        }
        else if (nParams == 1 && !namedIndex)
        {
            return bindParameter(stmt, 1, values);
        }
        else
        {
            mp_raise_msg_varg(&usqlite_Error,
                MP_ERROR_TEXT("tuple or list expected for > 1 nameless parameters, got a '%s'"),
                mp_obj_get_type_str(values));
            return -1;
        }

        if (!len)
        {
            mp_raise_ValueError(MP_ERROR_TEXT("Empty values set"));
            return -1;
        }

        for (size_t i = 0; i < nParams; i++)
        {
            size_t index = i;
            if (namedIndex)
            {
                name = sqlite3_bind_parameter_name(stmt, i + 1);
                if (!name)
                {
                    continue;
                }

                index = (size_t)atoi(name + 1) - 1;
            }

            if (index >= len)
            {
                mp_raise_msg_varg(&usqlite_Error, MP_ERROR_TEXT("Parameter index %d > %d values"), ++index, len);
                return -1;
            }

            int rc = bindParameter(stmt, i + 1, items[index]);
            if (rc)
            {
                return rc;
            }
        }
    }

    return SQLITE_OK;
}

// ------------------------------------------------------------------------------

static mp_obj_t usqlite_cursor_execute(size_t n_args, const mp_obj_t *args)
{
    mp_obj_t self_in = args[0];
    usqlite_cursor_t *self = MP_OBJ_TO_PTR(self_in);

    // SAFETY CHECK: Prevent using a closed cursor
    if (!self->connection) {
        mp_raise_ValueError(MP_ERROR_TEXT("Cursor is closed"));
        return mp_const_none;
    }

    const char *sql = mp_obj_str_get_str(args[1]);

    // Note: We DO NOT call close(self_in) here anymore because it would detach the connection!
    // Instead, we just finalize the *statement* to prepare for the new one.
    if (self->stmt) {
        sqlite3_finalize(self->stmt);
        self->stmt = NULL;
    }

    if (!sql || !*sql)
    {
        mp_raise_msg(&usqlite_Error, MP_ERROR_TEXT("Empty sql"));
        return mp_const_none;
    }

    int rc = sqlite3_prepare_v2(self->connection->db, sql, strlen(sql), &self->stmt, NULL);
    if (rc)
    {
        mp_raise_msg_varg(&usqlite_Error,
            MP_ERROR_TEXT("error (%d) %s preparing '%s'"),
            rc,
            sqlite3_errmsg(self->connection->db),
            sql);
        sqlite3_finalize(self->stmt);
        self->stmt = NULL;
        return mp_const_none;
    }

    int nParams = sqlite3_bind_parameter_count(self->stmt);
    if (nParams > 0)
    {
        if (n_args >= 3)
        {
            rc = bindParameters(self->stmt, args[2]);
            if (rc > 0)
            {
                mp_raise_msg_varg(&usqlite_Error,
                    MP_ERROR_TEXT("%s error binding '%s'"),
                    sqlite3_errstr(rc),
                    sql);

                return mp_const_none;
            }
            else if (rc < 0)
            {
                return mp_const_none;
            }
        }
        else
        {
            mp_raise_ValueError(MP_ERROR_TEXT("Values required"));
            return mp_const_none;
        }
    }

    self->rowcount = 0;

    stepExecute(self);

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

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(usqlite_cursor_execute_obj, 2, 3, usqlite_cursor_execute);

// ------------------------------------------------------------------------------

static mp_obj_t usqlite_cursor_executemany(mp_obj_t self_in, mp_obj_t sql_in)
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

static MP_DEFINE_CONST_FUN_OBJ_2(usqlite_cursor_executemany_obj, usqlite_cursor_executemany);

// ------------------------------------------------------------------------------

static mp_obj_t usqlite_cursor_getiter(mp_obj_t self_in, mp_obj_iter_buf_t* iter_buf)
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

// ------------------------------------------------------------------------------

static mp_obj_t row_dict(usqlite_cursor_t* cursor)
{
    int columns = sqlite3_data_count(cursor->stmt);

    mp_obj_t dict = mp_obj_new_dict(columns);

    for (int i = 0; i < columns; i++)
    {
        mp_obj_dict_store(dict, usqlite_column_name(cursor->stmt, i), usqlite_column_value(cursor->stmt, i));
    }

    return dict;
}

// ------------------------------------------------------------------------------

static mp_obj_t row_tuple(usqlite_cursor_t* cursor)
{
    int columns = sqlite3_data_count(cursor->stmt);

    mp_obj_tuple_t* o = MP_OBJ_TO_PTR(mp_obj_new_tuple(columns, NULL));

    for (int i = 0; i < columns; i++)
    {
        o->items[i] = usqlite_column_value(cursor->stmt, i);
    }

    return MP_OBJ_FROM_PTR(o);
}

// ------------------------------------------------------------------------------

static mp_obj_t row_type(usqlite_cursor_t* cursor)
{
    int columns = sqlite3_data_count(cursor->stmt);

    mp_obj_tuple_t* o = MP_OBJ_TO_PTR(mp_obj_new_tuple(columns + 1, NULL));

    o->items[columns] = MP_OBJ_FROM_PTR(cursor);

    for (int i = 0; i < columns; i++)
    {
        o->items[i] = usqlite_column_value(cursor->stmt, i);
    }

    return MP_OBJ_FROM_PTR(o);
}

// ------------------------------------------------------------------------------

static mp_obj_t usqlite_cursor_iternext(mp_obj_t self_in)
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
        stepExecute(self);
        break;

    case SQLITE_DONE:
        // self->rc = sqlite3_reset(self->stmt);
        break;

    case SQLITE_ERROR:
    default:
        mp_raise_msg_varg(&mp_type_RuntimeError, MP_ERROR_TEXT("sqlite3 error %d executing '%s'"), self->rc, sqlite3_sql(self->stmt));
        break;
    }

    return result;
}

// ------------------------------------------------------------------------------

static mp_obj_t usqlite_cursor_fetchone(mp_obj_t self_in)
{
    usqlite_cursor_t *self = MP_OBJ_TO_PTR(self_in);

    mp_obj_t result = self->rc == SQLITE_ROW
                          ? self->rowfactory(self)
                          : mp_const_none;

    if (self->rc == SQLITE_ROW)
    {
        stepExecute(self);
    }

    return result;
}

static MP_DEFINE_CONST_FUN_OBJ_1(usqlite_cursor_fetchone_obj, usqlite_cursor_fetchone);

// ------------------------------------------------------------------------------

static mp_obj_t usqlite_cursor_fetchmany(size_t n_args, const mp_obj_t* args)
{
    usqlite_cursor_t* self = MP_OBJ_TO_PTR(args[0]);

    if (self->rc != SQLITE_ROW)
    {
        return mp_obj_new_list(0, NULL);
    }

    mp_obj_t row = self->rowfactory(self);
    mp_obj_t list = mp_obj_new_list(1, &row);
    mp_obj_list_t* listt = MP_OBJ_TO_PTR(list);

    int size = n_args == 2
        ? mp_obj_get_int(args[1])
        : self->arraysize;

    stepExecute(args[0]);

    if (!size)
    {
        size = 1;
    }

    if (size == 1)
    {
        return list;
    }

    while (self->rc == SQLITE_ROW && (size < 0 || (int)listt->len < size))
    {
        row = self->rowfactory(self);
        mp_obj_list_append(list, row);
        stepExecute(self);
    }

    return list;
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(usqlite_cursor_fetchmany_obj, 1, 2, usqlite_cursor_fetchmany);

// ------------------------------------------------------------------------------

static mp_obj_t usqlite_cursor_fetchall(mp_obj_t self_in)
{
    mp_obj_t args[] =
    {
        self_in,
        mp_obj_new_int(-1) };

    return usqlite_cursor_fetchmany(MP_ARRAY_SIZE(args), args);
}

static MP_DEFINE_CONST_FUN_OBJ_1(usqlite_cursor_fetchall_obj, usqlite_cursor_fetchall);

// ------------------------------------------------------------------------------

static mp_obj_t usqlite_cursor_description(sqlite3_stmt* stmt)
{
    int columns = sqlite3_data_count(stmt);

    mp_obj_tuple_t* o = MP_OBJ_TO_PTR(mp_obj_new_tuple(columns, NULL));

    for (int i = 0; i < columns; i++)
    {
        mp_obj_tuple_t* c = MP_OBJ_TO_PTR(mp_obj_new_tuple(7, NULL));

        c->items[0] = usqlite_column_name(stmt, i);
#ifndef SQLITE_OMIT_DECLTYPE
        c->items[1] = usqlite_column_decltype(stmt, i);
#else
        c->items[1] = mp_const_none;
#endif
        for (int j = 2; j < 7; j++)
        {
            c->items[j] = mp_const_none;
        }

        o->items[i] = MP_OBJ_FROM_PTR(c);
    }

    return MP_OBJ_FROM_PTR(o);
}

// ------------------------------------------------------------------------------

static void usqlite_cursor_attr(mp_obj_t self_in, qstr attr, mp_obj_t* dest)
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
        case MP_QSTR_connection:
            dest[0] = MP_OBJ_FROM_PTR(self->connection);
            break;

        case MP_QSTR_description:
            dest[0] = usqlite_cursor_description(self->stmt);
            break;

        case MP_QSTR_lastrowid:
        {
            sqlite3_int64 rowid = sqlite3_last_insert_rowid(self->connection->db);
            dest[0] = rowid ? mp_obj_new_int_from_ll(rowid) : mp_const_none;
        }
        break;

        case MP_QSTR_rowcount:
            dest[0] = mp_obj_new_int(self->rowcount);
            break;

        case MP_QSTR_arraysize:
            dest[0] = mp_obj_new_int(self->arraysize);
            break;
        }
    }
    else if (dest[1] != MP_OBJ_NULL)
    {
        switch (attr)
        {
        case MP_QSTR_arraysize:
            self->arraysize = mp_obj_get_int(dest[1]);
            dest[0] = MP_OBJ_NULL;
            break;
        }
    }
}
// ------------------------------------------------------------------------------

static mp_obj_t usqlite_cursor_del(mp_obj_t self_in)
{
    usqlite_logprintf(___FUNC___ "\n");
    // Just call close(), which now handles the deregistration logic safely
    return usqlite_cursor_close(self_in);
}

MP_DEFINE_CONST_FUN_OBJ_1(usqlite_cursor_del_obj, usqlite_cursor_del);

// ------------------------------------------------------------------------------

static mp_obj_t usqlite_cursor_exit(size_t n_args, const mp_obj_t* args)
{
    usqlite_logprintf(___FUNC___ "\n");

    usqlite_cursor_close(args[0]);

    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(usqlite_cursor_exit_obj, 4, 4, usqlite_cursor_exit);

// ------------------------------------------------------------------------------

static const mp_rom_map_elem_t usqlite_cursor_locals_dict_table[] =
{
    {MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&usqlite_cursor_del_obj)},
    {MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&mp_identity_obj)},
    {MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&usqlite_cursor_exit_obj)},

    {MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&usqlite_cursor_close_obj)},
    {MP_ROM_QSTR(MP_QSTR_execute), MP_ROM_PTR(&usqlite_cursor_execute_obj)},
    {MP_ROM_QSTR(MP_QSTR_executemany), MP_ROM_PTR(&usqlite_cursor_executemany_obj)},
    {MP_ROM_QSTR(MP_QSTR_fetchone), MP_ROM_PTR(&usqlite_cursor_fetchone_obj)},
    {MP_ROM_QSTR(MP_QSTR_fetchmany), MP_ROM_PTR(&usqlite_cursor_fetchmany_obj)},
    {MP_ROM_QSTR(MP_QSTR_fetchall), MP_ROM_PTR(&usqlite_cursor_fetchall_obj)},
};

MP_DEFINE_CONST_DICT(usqlite_cursor_locals_dict, usqlite_cursor_locals_dict_table);

// ------------------------------------------------------------------------------

#if defined(MP_DEFINE_CONST_OBJ_TYPE)
static const mp_getiter_iternext_custom_t usqlite_getiter_iternext = {
    .getiter = usqlite_cursor_getiter,
    .iternext = usqlite_cursor_iternext,
};

MP_DEFINE_CONST_OBJ_TYPE(
    usqlite_cursor_type,
    MP_QSTR_Cursor,
    MP_TYPE_FLAG_ITER_IS_CUSTOM,
    make_new, usqlite_cursor_make_new,
    print, usqlite_cursor_print,
    attr, usqlite_cursor_attr,
    iter, &usqlite_getiter_iternext,
    locals_dict, &usqlite_cursor_locals_dict);
#else
const mp_obj_type_t usqlite_cursor_type =
{
    {&mp_type_type},
    .name = MP_QSTR_Cursor,
    .print = usqlite_cursor_print,
    .make_new = usqlite_cursor_make_new,
    .getiter = usqlite_cursor_getiter,
    .iternext = usqlite_cursor_iternext,
    .locals_dict = (mp_obj_dict_t*)&usqlite_cursor_locals_dict,
    .attr = &usqlite_cursor_attr };
#endif

// ------------------------------------------------------------------------------
