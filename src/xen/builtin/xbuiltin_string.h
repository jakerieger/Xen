#ifndef X_BUILTIN_STRING_H
#define X_BUILTIN_STRING_H

#include "../xcommon.h"
#include "../xvalue.h"

xen_obj_namespace* xen_builtin_string();

xen_value xen_str_len(i32 argc, xen_value* argv);
xen_value xen_str_upper(i32 argc, xen_value* argv);
xen_value xen_str_lower(i32 argc, xen_value* argv);
xen_value xen_str_trim(i32 argc, xen_value* argv);
xen_value xen_str_contains(i32 argc, xen_value* argv);
xen_value xen_str_starts_with(i32 argc, xen_value* argv);
xen_value xen_str_ends_with(i32 argc, xen_value* argv);
xen_value xen_str_substr(i32 argc, xen_value* argv);
xen_value xen_str_find(i32 argc, xen_value* argv);
xen_value xen_str_split(i32 argc, xen_value* argv);
xen_value xen_str_replace(i32 argc, xen_value* argv);

#endif