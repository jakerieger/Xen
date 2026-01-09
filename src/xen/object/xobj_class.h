#ifndef X_OBJ_CLASS_H
#define X_OBJ_CLASS_H

#include "xobj.h"
#include "xobj_native_function.h"

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

    array(xen_property_def) properties;
    i32 property_count;
    i32 property_capacity;

    xen_table methods;          // name -> xen_obj_func*
    xen_table private_methods;  // name -> xen_obj_func*

    xen_obj_func* initializer;         // init() method
    xen_native_fn native_initializer;  // init() for native classes
};

#define OBJ_IS_CLASS(value) xen_obj_is_type(value, OBJ_CLASS)
#define OBJ_AS_CLASS(value) ((xen_obj_class*)VAL_AS_OBJ(value))

xen_obj_class* xen_obj_class_new(xen_obj_str* name);
void xen_obj_class_add_property(xen_obj_class* class, xen_obj_str* name, xen_value default_val, bool is_private);
void xen_obj_class_add_method(xen_obj_class* class, xen_obj_str* name, xen_obj_func* method, bool is_private);

i32 xen_find_property_index(xen_obj_class* class, xen_obj_str* name);
bool xen_obj_class_is_property_private(xen_obj_class* class, xen_obj_str* name);
void xen_obj_class_set_native_init(xen_obj_class* class, xen_native_fn init_fn);
void xen_obj_class_add_native_method(xen_obj_class* class, const char* name, xen_native_fn method, bool is_private);

#endif