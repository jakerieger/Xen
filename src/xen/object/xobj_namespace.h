#ifndef X_OBJ_NAMESPACE_H
#define X_OBJ_NAMESPACE_H

#include "xobj.h"

struct xen_ns_entry {
    const char* name;
    xen_value value;
};

struct xen_obj_namespace {
    xen_obj obj;
    const char* name;
    array(xen_ns_entry) entries;
    i32 count;
    i32 capacity;
};

#define OBJ_IS_NAMESPACE(v) xen_obj_is_type(v, OBJ_NAMESPACE)
#define OBJ_AS_NAMESPACE(v) ((xen_obj_namespace*)VAL_AS_OBJ(v))

xen_obj_namespace* xen_obj_namespace_new(const char* name);
void xen_obj_namespace_set(xen_obj_namespace* ns, const char* name, xen_value value);
bool xen_obj_namespace_get(xen_obj_namespace* ns, const char* name, xen_value* out);

#endif