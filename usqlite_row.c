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

static void usqlite_row_attr(mp_obj_t self_in, qstr attr, mp_obj_t* dest);

// ------------------------------------------------------------------------------

void usqlite_row_type_initialize() {
    static int initialized = 0;

    if (initialized) {
        return;
    }

#if defined(MP_DEFINE_CONST_OBJ_TYPE)
    usqlite_row_type.base.type = &mp_type_type;
    usqlite_row_type.flags = MP_TYPE_FLAG_ITER_IS_GETITER;
    usqlite_row_type.name = MP_QSTR_Row;
    MP_OBJ_TYPE_SET_SLOT(&usqlite_row_type, make_new, MP_OBJ_TYPE_GET_SLOT(&mp_type_tuple, make_new), 0);
    MP_OBJ_TYPE_SET_SLOT(&usqlite_row_type, print, mp_obj_tuple_print, 1);
    MP_OBJ_TYPE_SET_SLOT(&usqlite_row_type, unary_op, mp_obj_tuple_unary_op, 2);
    MP_OBJ_TYPE_SET_SLOT(&usqlite_row_type, binary_op, mp_obj_tuple_binary_op, 3);
    MP_OBJ_TYPE_SET_SLOT(&usqlite_row_type, attr, usqlite_row_attr, 4);
    MP_OBJ_TYPE_SET_SLOT(&usqlite_row_type, subscr, mp_obj_tuple_subscr, 5);
    MP_OBJ_TYPE_SET_SLOT(&usqlite_row_type, iter, mp_obj_tuple_getiter, 6);
    MP_OBJ_TYPE_SET_SLOT(&usqlite_row_type, locals_dict, MP_OBJ_TYPE_GET_SLOT(&mp_type_tuple, locals_dict), 7);
#else
    usqlite_row_type.make_new = mp_type_tuple.make_new;
    usqlite_row_type.locals_dict = mp_type_tuple.locals_dict;
#endif

    initialized = 1;
}

// ------------------------------------------------------------------------------

static mp_obj_t keys(usqlite_cursor_t* cursor) {
    if (!cursor || !cursor->stmt) {
        return mp_const_none;
    }

    int columns = sqlite3_data_count(cursor->stmt);

    mp_obj_tuple_t* o = MP_OBJ_TO_PTR(mp_obj_new_tuple(columns, NULL));

    for (int i = 0; i < columns; i++)
    {
        o->items[i] = usqlite_column_name(cursor->stmt, i);
    }

    return MP_OBJ_FROM_PTR(o);
}

// ------------------------------------------------------------------------------

static void usqlite_row_attr(mp_obj_t self_in, qstr attr, mp_obj_t* dest) {
    if (dest[0] == MP_OBJ_NULL) {
        if ((usqlite_lookup(self_in, attr, dest))) {
            return;
        }

        mp_obj_tuple_t* self = (mp_obj_tuple_t*)self_in;

        switch (attr)
        {
        case MP_QSTR_keys:
            // Validate that the tuple has the cursor pointer
            if (self->len > 0 && self->items[self->len - 1] != MP_OBJ_NULL) {
                usqlite_cursor_t* cursor = MP_OBJ_TO_PTR(self->items[self->len - 1]);
                if (cursor && cursor->stmt) {
                    dest[0] = keys(cursor);
                }
            }
            break;
        }
    }
}

// ------------------------------------------------------------------------------

#if defined(MP_DEFINE_CONST_OBJ_TYPE)
mp_obj_full_type_t usqlite_row_type;
#else
mp_obj_type_t usqlite_row_type =
{
    { &mp_type_type },
    .name = MP_QSTR_Row,
    .print = mp_obj_tuple_print,
    .make_new = NULL,
    .unary_op = mp_obj_tuple_unary_op,
    .binary_op = mp_obj_tuple_binary_op,
    .subscr = mp_obj_tuple_subscr,
    .getiter = mp_obj_tuple_getiter,
    .locals_dict = NULL,
    .attr = usqlite_row_attr
};
#endif

// ------------------------------------------------------------------------------
