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

// ------------------------------------------------------------------------------

static mp_obj_t usqlite_connection_close(mp_obj_t self_in);

// ------------------------------------------------------------------------------

static mp_obj_t usqlite_connection_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // Allocated with a finaliser so __del__ really runs when an unclosed
    // connection is collected (a plain m_new_obj has none, which made the
    // __del__ below dead code and left the db handle -- page cache and all --
    // in the SQLite pool for the rest of the session).
    usqlite_connection_t *self = mp_obj_malloc_with_finaliser(usqlite_connection_t, &usqlite_connection_type);

    self->db = (sqlite3 *)MP_OBJ_TO_PTR(args[0]);
    self->row_type = MP_QSTR_tuple;
    self->row_factory = mp_const_none;
    self->trace_callback = mp_const_none;
    mp_obj_list_init(&self->cursors, 0);

    return MP_OBJ_FROM_PTR(self);
}

// ------------------------------------------------------------------------------

static void usqlite_connection_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    usqlite_connection_t *self = MP_OBJ_TO_PTR(self_in);

    mp_printf(print, "<%s '%s'>", mp_obj_get_type_str(self_in), self->db ? sqlite3_db_filename(self->db, NULL) : "");
}

// ------------------------------------------------------------------------------

static mp_obj_t usqlite_connection_close(mp_obj_t self_in) {
    LOGFUNC;

    usqlite_connection_t *self = (usqlite_connection_t *)MP_OBJ_TO_PTR(self_in);
    if (!self->db) {
        return mp_const_none;
    }

    // Finalize the statement of every cursor still tracked. The cursor
    // objects themselves belong to the GC -- Python code may well still hold
    // references to them -- so they are never freed here (m_free of a live
    // object was a use-after-free); the collector reclaims each one once it
    // goes unreferenced. Drain from the tail, unhooking before closing.
    while (self->cursors.len) {
        mp_obj_t cursor = self->cursors.items[--self->cursors.len];
        ((usqlite_cursor_t *)MP_OBJ_TO_PTR(cursor))->registered = false;
        usqlite_cursor_close(cursor);
    }

    usqlite_logprintf(___FUNC___ " closing '%s'\n", sqlite3_db_filename(self->db, NULL));
    // close_v2: if anything is somehow still unfinalized, the handle becomes a
    // zombie freed when the last statement goes, instead of leaking outright.
    sqlite3_close_v2(self->db);
    self->db = NULL;

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(usqlite_connection_close_obj, usqlite_connection_close);

// ------------------------------------------------------------------------------

static mp_obj_t usqlite_connection_del(mp_obj_t self_in) {
    usqlite_logprintf(___FUNC___, "\n");

    usqlite_connection_close(self_in);

    return mp_const_none;
};

MP_DEFINE_CONST_FUN_OBJ_1(usqlite_connection_del_obj, usqlite_connection_del);

// ------------------------------------------------------------------------------

static mp_obj_t usqlite_connection_cursor(mp_obj_t self_in) {
    usqlite_connection_t *self = MP_OBJ_TO_PTR(self_in);

    mp_obj_t args[2] =
    {
        MP_OBJ_FROM_PTR(self),
        mp_const_none
    };

    #if defined(MP_OBJ_TYPE_GET_SLOT)
    return MP_OBJ_TYPE_GET_SLOT(&usqlite_cursor_type, make_new)(NULL, MP_ARRAY_SIZE(args), 0, args);
    #else
    return usqlite_cursor_type.make_new(NULL, MP_ARRAY_SIZE(args), 0, args);
    #endif
}

static MP_DEFINE_CONST_FUN_OBJ_1(usqlite_connection_cursor_obj, usqlite_connection_cursor);

// ------------------------------------------------------------------------------

static mp_obj_t usqlite_connection_execute(size_t n_args, const mp_obj_t *args) {
    usqlite_connection_t *self = MP_OBJ_TO_PTR(args[0]);

    mp_obj_t xargs[4] =
    {
        MP_OBJ_FROM_PTR(self),
        mp_const_false,
        args[1]
    };

    size_t nxargs = 3;

    if (n_args == 3) {
        xargs[3] = args[2];
        nxargs++;
    }

    #if defined(MP_OBJ_TYPE_GET_SLOT)
    return MP_OBJ_TYPE_GET_SLOT(&usqlite_cursor_type, make_new)(NULL, nxargs, 0, xargs);
    #else
    return usqlite_cursor_type.make_new(NULL, nxargs, 0, xargs);
    #endif
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(usqlite_connection_execute_obj, 2, 3, usqlite_connection_execute);

// ------------------------------------------------------------------------------

static mp_obj_t usqlite_connection_executemany(mp_obj_t self_in, mp_obj_t sql) {
    usqlite_connection_t *self = MP_OBJ_TO_PTR(self_in);

    mp_obj_t args[3] =
    {
        MP_OBJ_FROM_PTR(self),
        mp_const_true,
        sql
    };

    #if defined(MP_OBJ_TYPE_GET_SLOT)
    return MP_OBJ_TYPE_GET_SLOT(&usqlite_cursor_type, make_new)(NULL, MP_ARRAY_SIZE(args), 0, args);
    #else
    return usqlite_cursor_type.make_new(NULL, MP_ARRAY_SIZE(args), 0, args);
    #endif
}

static MP_DEFINE_CONST_FUN_OBJ_2(usqlite_connection_executemany_obj, usqlite_connection_executemany);

// ------------------------------------------------------------------------------

static int traceCallback(unsigned uMask, void *context, void *p, void *x) {
    usqlite_connection_t *self = (usqlite_connection_t *)context;
    sqlite3_stmt *stmt = (sqlite3_stmt *)p;
    char *xsql = sqlite3_expanded_sql(stmt);
    const char *sql = xsql ? xsql : sqlite3_sql(stmt);

    // The callback runs from inside sqlite3_step; a raise in user code would
    // longjmp through the VDBE's C frames (and leak xsql). Swallow it, like
    // CPython's trace callbacks do.
    if (sql) {
        nlr_buf_t nlr;
        if (nlr_push(&nlr) == 0) {
            mp_call_function_1(self->trace_callback, mp_obj_new_str(sql, strlen(sql)));
            nlr_pop();
        }
    }
    sqlite3_free(xsql);

    return 0;
}

static mp_obj_t usqlite_connection_set_trace_callback(mp_obj_t self_in, mp_obj_t callback) {
    usqlite_connection_t *self = MP_OBJ_TO_PTR(self_in);

    if (callback == mp_const_none) {
        self->trace_callback = mp_const_none;
        sqlite3_trace_v2(self->db, 0, NULL, NULL);
    } else if (mp_obj_is_callable(callback)) {
        self->trace_callback = callback;
        sqlite3_trace_v2(self->db, SQLITE_TRACE_STMT, traceCallback, self);
    } else {
        mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("Invalid callback"));
    }

    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_2(usqlite_connection_set_trace_callback_obj, usqlite_connection_set_trace_callback);

// ------------------------------------------------------------------------------

void usqlite_connection_register(usqlite_connection_t *connection, mp_obj_t cursor) {
    mp_obj_t cursors = MP_OBJ_FROM_PTR(&connection->cursors);
    mp_obj_list_append(cursors, cursor);
}

// ------------------------------------------------------------------------------

void usqlite_connection_deregister(usqlite_connection_t *connection, mp_obj_t cursor) {
    // Identity swap-remove that never raises: mp_obj_list_remove raises when
    // the item is absent, and this runs on close/finaliser paths where a raise
    // must not happen.
    mp_obj_list_t *l = &connection->cursors;
    for (size_t i = 0; i < l->len; i++) {
        if (l->items[i] == cursor) {
            l->items[i] = l->items[l->len - 1];
            l->items[l->len - 1] = MP_OBJ_NULL;
            l->len--;
            return;
        }
    }
}

// ------------------------------------------------------------------------------

static void usqlite_connection_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    usqlite_connection_t *self = (usqlite_connection_t *)self_in;

    if (dest[0] == MP_OBJ_NULL) {
        if ((usqlite_lookup(self_in, attr, dest))) {
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
    } else if (dest[1] != MP_OBJ_NULL) {
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

// ------------------------------------------------------------------------------

static mp_obj_t usqlite_connection_exit(size_t n_args, const mp_obj_t *args) {
    usqlite_logprintf(___FUNC___ "\n");

    usqlite_connection_close(args[0]);

    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(usqlite_connection_exit_obj, 4, 4, usqlite_connection_exit);

// ------------------------------------------------------------------------------

static const mp_rom_map_elem_t usqlite_connection_locals_dict_table[] =
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

// ------------------------------------------------------------------------------

#if defined(MP_DEFINE_CONST_OBJ_TYPE)
MP_DEFINE_CONST_OBJ_TYPE(
    usqlite_connection_type,
    MP_QSTR_Connection,
    MP_TYPE_FLAG_NONE,
    make_new, usqlite_connection_make_new,
    print, usqlite_connection_print,
    attr, usqlite_connection_attr,
    locals_dict, &usqlite_connection_locals_dict
    );
#else
const mp_obj_type_t usqlite_connection_type =
{
    { &mp_type_type },
    .name = MP_QSTR_Connection,
    .print = usqlite_connection_print,
    .make_new = usqlite_connection_make_new,
    .locals_dict = (mp_obj_dict_t *)&usqlite_connection_locals_dict,
    .attr = usqlite_connection_attr,
};
#endif

// ------------------------------------------------------------------------------
