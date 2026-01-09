#ifndef X_BUILTIN_DICT_H
#define X_BUILTIN_DICT_H

#include "../xcommon.h"
#include "../xvalue.h"

xen_obj_namespace* xen_builtin_dict();

xen_value xen_dict_len(i32 argc, xen_value* argv);
xen_value xen_dict_keys(i32 argc, xen_value* argv);
xen_value xen_dict_values(i32 argc, xen_value* argv);
xen_value xen_dict_has(i32 argc, xen_value* argv);
xen_value xen_dict_remove(i32 argc, xen_value* argv);
xen_value xen_dict_clear(i32 argc, xen_value* argv);

#endif