#ifndef X_OBJ_STRING_H
#define X_OBJ_STRING_H

#include "xobj.h"
#include "../builtin/xbuiltin_string.h"

struct xen_obj_str {
    xen_obj obj;
    i32 length;
    char* str;
    u32 hash;
};

#define OBJ_IS_STRING(v) xen_obj_is_type(v, OBJ_STRING)
#define OBJ_AS_STRING(v) ((xen_obj_str*)VAL_AS_OBJ(v))
#define OBJ_AS_CSTRING(v) (((xen_obj_str*)VAL_AS_OBJ(v))->str)
#define EMPTY_STRING_VAL (OBJ_VAL(xen_obj_str_copy("", 0)))

xen_obj_str* xen_obj_str_take(char* chars, i32 length);
xen_obj_str* xen_obj_str_copy(const char* chars, i32 length);

static xen_method_entry k_string_methods[] = {{"len", xen_str_len, XEN_TRUE},  // property
                                              {"upper", xen_str_upper, XEN_FALSE},
                                              {"lower", xen_str_lower, XEN_FALSE},
                                              {"trim", xen_str_trim, XEN_FALSE},
                                              {"contains", xen_str_contains, XEN_FALSE},
                                              {"starts_with", xen_str_starts_with, XEN_FALSE},
                                              {"ends_with", xen_str_ends_with, XEN_FALSE},
                                              {"substr", xen_str_substr, XEN_FALSE},
                                              {"find", xen_str_find, XEN_FALSE},
                                              {"split", xen_str_split, XEN_FALSE},
                                              {"replace", xen_str_replace, XEN_FALSE},
                                              {NULL, NULL, XEN_FALSE}};

#endif
