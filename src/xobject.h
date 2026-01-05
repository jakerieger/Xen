#ifndef X_OBJECT_H
#define X_OBJECT_H

#include "xvalue.h"
#include "xchunk.h"
#include "xtable.h"

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
} xen_obj_type;

struct xen_obj {
    xen_obj_type type;
    xen_obj* next;
};

struct xen_obj_str {
    xen_obj obj;
    i32 length;
    char* str;
    u32 hash;
};

struct xen_obj_func {
    xen_obj obj;
    i32 arity;  // number of parameters
    xen_chunk chunk;
    xen_obj_str* name;
};

typedef xen_value (*xen_native_fn)(i32 arg_count, xen_value* args);

struct xen_obj_native_func {
    xen_obj obj;
    xen_native_fn function;
    const char* name;
};

struct xen_ns_entry {
    const char* name;
    xen_value value;
};

struct xen_obj_namespace {
    xen_obj obj;
    const char* name;
    xen_ns_entry* entries;
    i32 count;
    i32 capacity;
};

struct xen_obj_array {
    xen_obj obj;
    xen_value_array array;
};

struct xen_obj_bound_method {
    xen_obj obj;
    xen_value receiver;    // The object the method is bound to
    xen_native_fn method;  // The native method function
    xen_obj_func* function;
    const char* name;  // Method name for debugging
};

struct xen_obj_dict {
    xen_obj obj;
    xen_table table;
};

typedef struct {
    xen_obj_str* name;
    xen_value default_value;
    bool is_private;
    i32 index;  // index in instance fields array
} xen_property_def;

// class definition (the blueprint)
struct xen_obj_class {
    xen_obj obj;
    xen_obj_str* name;

    xen_property_def* properties;
    i32 property_count;
    i32 property_capacity;

    xen_table methods;          // name -> xen_obj_func*
    xen_table private_methods;  // name -> xen_obj_func*

    xen_obj_func* initializer;  // init() method
};

typedef struct {
    xen_obj obj;
    xen_obj_class* class;
    xen_value* fields;  // property values, indexed by propery_def.index
} xen_obj_instance;

#define OBJ_TYPE(v) (VAL_AS_OBJ(v)->type)

#define OBJ_IS_STRING(v) xen_obj_is_type(v, OBJ_STRING)
#define OBJ_AS_STRING(v) ((xen_obj_str*)VAL_AS_OBJ(v))
#define OBJ_AS_CSTRING(v) (((xen_obj_str*)VAL_AS_OBJ(v))->str)

#define OBJ_IS_FUNCTION(v) xen_obj_is_type(v, OBJ_FUNCTION)
#define OBJ_AS_FUNCTION(v) ((xen_obj_func*)VAL_AS_OBJ(v))

#define OBJ_IS_NATIVE_FUNC(v) xen_obj_is_type(v, OBJ_NATIVE_FUNC)
#define OBJ_AS_NATIVE_FUNC(v) ((xen_obj_native_func*)VAL_AS_OBJ(v))

#define OBJ_IS_NAMESPACE(v) xen_obj_is_type(v, OBJ_NAMESPACE)
#define OBJ_AS_NAMESPACE(v) ((xen_obj_namespace*)VAL_AS_OBJ(v))

#define OBJ_IS_ARRAY(v) xen_obj_is_type(v, OBJ_ARRAY)
#define OBJ_AS_ARRAY(v) ((xen_obj_array*)VAL_AS_OBJ(v))

#define OBJ_IS_BOUND_METHOD(v) xen_obj_is_type(v, OBJ_BOUND_METHOD)
#define OBJ_AS_BOUND_METHOD(v) ((xen_obj_bound_method*)VAL_AS_OBJ(v))

#define OBJ_IS_DICT(value) xen_obj_is_type(value, OBJ_DICT)
#define OBJ_AS_DICT(value) ((xen_obj_dict*)VAL_AS_OBJ(value))

#define OBJ_IS_CLASS(value) xen_obj_is_type(value, OBJ_CLASS)
#define OBJ_IS_INSTANCE(value) xen_obj_is_type(value, OBJ_INSTANCE)
#define OBJ_AS_CLASS(value) ((xen_obj_class*)VAL_AS_OBJ(value))
#define OBJ_AS_INSTANCE(value) ((xen_obj_instance*)VAL_AS_OBJ(value))

bool xen_obj_is_type(xen_value value, xen_obj_type type);
void xen_obj_print(xen_value value);

xen_obj_str* xen_obj_str_take(char* chars, i32 length);
xen_obj_str* xen_obj_str_copy(const char* chars, i32 length);

xen_obj_func* xen_obj_func_new();
xen_obj_native_func* xen_obj_native_func_new(xen_native_fn function, const char* name);

xen_obj_namespace* xen_obj_namespace_new(const char* name);
void xen_obj_namespace_set(xen_obj_namespace* ns, const char* name, xen_value value);
bool xen_obj_namespace_get(xen_obj_namespace* ns, const char* name, xen_value* out);

xen_obj_array* xen_obj_array_new();
xen_obj_array* xen_obj_array_new_with_capacity(i32 capacity);
void xen_obj_array_push(xen_obj_array* arr, xen_value value);
xen_value xen_obj_array_get(xen_obj_array* arr, i32 index);
void xen_obj_array_set(xen_obj_array* arr, i32 index, xen_value value);
xen_value xen_obj_array_pop(xen_obj_array* arr);
i32 xen_obj_array_length(xen_obj_array* arr);

// Bound method functions
xen_obj_bound_method* xen_obj_bound_method_new(xen_value receiver, xen_native_fn method, const char* name);
xen_obj_bound_method* xen_obj_bound_method_new_func(xen_value receiver, xen_obj_func* func);

// Method entry for method tables
typedef struct {
    const char* name;
    xen_native_fn method;
    bool is_property;  // true = no parens needed (e.g., .len), false = callable
} xen_method_entry;

// Lookup a method on a value by name. Returns NULL if not found.
xen_native_fn xen_lookup_method(xen_value value, const char* name, bool* is_property);

xen_obj_dict* xen_obj_dict_new();
void xen_obj_dict_set(xen_obj_dict* dict, xen_value key, xen_value value);
bool xen_obj_dict_get(xen_obj_dict* dict, xen_value key, xen_value* out);
bool xen_obj_dict_delete(xen_obj_dict* dict, xen_value key);

xen_obj_class* xen_obj_class_new(xen_obj_str* name);
void xen_obj_class_add_property(xen_obj_class* class, xen_obj_str* name, xen_value default_val, bool is_private);
void xen_obj_class_add_method(xen_obj_class* class, xen_obj_str* name, xen_obj_func* method, bool is_private);

xen_obj_instance* xen_obj_instance_new(xen_obj_class* class);
bool xen_obj_instance_get(xen_obj_instance* inst, xen_obj_str* name, xen_value* out);
bool xen_obj_instance_set(xen_obj_instance* inst, xen_obj_str* name, xen_value value);

#endif
