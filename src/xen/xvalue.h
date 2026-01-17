#ifndef X_VALUE_H
#define X_VALUE_H

/**
 * Defines common value types used by Xen
 */

#include "xcommon.h"

// xobject.h forward declarations
// clang-format off
typedef struct xen_obj               xen_obj;
typedef struct xen_obj_str           xen_obj_str;
typedef struct xen_obj_func          xen_obj_func;
typedef struct xen_obj_native_func   xen_obj_native_func;
typedef struct xen_ns_entry          xen_ns_entry;
typedef struct xen_obj_namespace     xen_obj_namespace;
typedef struct xen_obj_array         xen_obj_array;
typedef struct xen_obj_bound_method  xen_obj_bound_method;
typedef struct xen_obj_dict          xen_obj_dict;
typedef struct xen_obj_class         xen_obj_class;
typedef struct xen_obj_u8array       xen_obj_u8array;
typedef struct xen_obj_error         xen_obj_error;
// clang-format on

typedef enum {
    VAL_BOOL,
    VAL_NULL,
    VAL_NUMBER,
    VAL_OBJECT,
} xen_value_type;

typedef struct {
    xen_value_type type;

    union {
        bool boolean;
        f64 number;
        xen_obj* obj;
    } as;
} xen_value;

typedef struct {
    u64 cap;
    u64 count;
    array(xen_value) values;
} xen_value_array;

#define VAL_IS_BOOL(v) ((v).type == VAL_BOOL)
#define VAL_IS_NULL(v) ((v).type == VAL_NULL)
#define VAL_IS_NUMBER(v) ((v).type == VAL_NUMBER)
#define VAL_IS_OBJ(v) ((v).type == VAL_OBJECT)

#define VAL_AS_OBJ(v) ((v).as.obj)
#define VAL_AS_BOOL(v) ((v).as.boolean)
#define VAL_AS_NUMBER(v) ((v).as.number)

#define NULL_VAL ((xen_value) {VAL_NULL, {.number = 0}})
#define BOOL_VAL(v) ((xen_value) {VAL_BOOL, {.boolean = (v)}})
#define NUMBER_VAL(v) ((xen_value) {VAL_NUMBER, {.number = (v)}})
#define OBJ_VAL(o) ((xen_value) {VAL_OBJECT, {.obj = (xen_obj*)(o)}})

bool xen_value_equal(xen_value a, xen_value b);
void xen_value_array_init(xen_value_array* array);
void xen_value_array_write(xen_value_array* array, xen_value value);
void xen_value_array_free(xen_value_array* array);
void xen_value_print(xen_value value);

char* xen_value_type_to_str(xen_value value);

#endif
