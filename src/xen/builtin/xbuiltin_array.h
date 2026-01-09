#ifndef X_BUILTIN_ARRAY_H
#define X_BUILTIN_ARRAY_H

#include "../xcommon.h"
#include "../xvalue.h"

xen_obj_namespace* xen_builtin_array();

xen_value xen_arr_len(i32 argc, xen_value* argv);
xen_value xen_arr_push(i32 argc, xen_value* argv);
xen_value xen_arr_pop(i32 argc, xen_value* argv);
xen_value xen_arr_first(i32 argc, xen_value* argv);
xen_value xen_arr_last(i32 argc, xen_value* argv);
xen_value xen_arr_clear(i32 argc, xen_value* argv);
xen_value xen_arr_contains(i32 argc, xen_value* argv);
xen_value xen_arr_index_of(i32 argc, xen_value* argv);
xen_value xen_arr_reverse(i32 argc, xen_value* argv);
xen_value xen_arr_join(i32 argc, xen_value* argv);

#endif