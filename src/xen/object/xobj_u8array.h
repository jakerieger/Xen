#ifndef X_OBJ_U8ARRAY_H
#define X_OBJ_U8ARRAY_H

#include "xobj.h"

struct xen_obj_u8array {
    xen_obj obj;
    array(u8) values;
    i32 count;
    i32 capacity;
};

#define OBJ_IS_U8ARRAY(value) xen_obj_is_type(value, OBJ_U8ARRAY)
#define OBJ_AS_U8ARRAY(value) ((xen_obj_u8array*)VAL_AS_OBJ(value))

xen_obj_u8array* xen_obj_u8array_new();
xen_obj_u8array* xen_obj_u8array_new_with_capacity(i32 capacity);
void xen_obj_u8array_push(xen_obj_u8array* arr, u8 value);
u8 xen_obj_u8array_get(xen_obj_u8array* arr, i32 index);
void xen_obj_u8array_set(xen_obj_u8array* arr, i32 index, u8 value);
u8 xen_u8obj_array_pop(xen_obj_u8array* arr);
i32 xen_obj_u8array_length(xen_obj_u8array* arr);

#endif