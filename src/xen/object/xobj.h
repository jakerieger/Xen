#ifndef X_OBJ_H
#define X_OBJ_H

#include "../xvalue.h"
#include "../xchunk.h"
#include "../xtable.h"

typedef enum {
    OBJ_STRING,
    OBJ_FUNCTION,
    OBJ_NATIVE_FUNC,
    OBJ_NAMESPACE,
    OBJ_ARRAY,
    OBJ_BOUND_METHOD,  // for property access that returns a callable
    OBJ_DICT,
    OBJ_CLASS,
    OBJ_INSTANCE,
    OBJ_U8ARRAY,
    OBJ_ERROR,
} xen_obj_type;

struct xen_obj {
    xen_obj_type type;
    xen_obj* next;
};

typedef xen_value (*xen_native_fn)(i32 arg_count, xen_value* args);

#define ALLOCATE_OBJ(type, obj_type) (type*)xen_obj_allocate(sizeof(type), obj_type);
xen_obj* xen_obj_allocate(size_t size, xen_obj_type type);

#define OBJ_TYPE(v) (VAL_AS_OBJ(v)->type)

bool xen_obj_is_type(xen_value value, xen_obj_type type);
void xen_obj_print(xen_value value);

typedef struct {
    const char* name;
    xen_native_fn method;
    bool is_property;  // true = no parens needed (e.g., .len), false = callable
} xen_method_entry;

// Lookup a method on a value by name. Returns NULL if not found.
xen_native_fn xen_lookup_method(xen_value value, const char* name, bool* is_property);

#endif
