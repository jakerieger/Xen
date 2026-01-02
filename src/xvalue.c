#include "xvalue.h"
#include "xmem.h"
#include "xobject.h"

bool xen_value_equal(xen_value a, xen_value b) {
    if (a.type != b.type)
        return XEN_FALSE;

    switch (a.type) {
        case VAL_BOOL:
            return VAL_AS_BOOL(a) == VAL_AS_BOOL(b);
        case VAL_NUMBER:
            return VAL_AS_NUMBER(a) == VAL_AS_NUMBER(b);
        case VAL_OBJECT: {
            xen_obj* obj_a = VAL_AS_OBJ(a);
            xen_obj* obj_b = VAL_AS_OBJ(b);

            /* Same pointer = definitely equal */
            if (obj_a == obj_b)
                return XEN_TRUE;

            /* Different types = not equal */
            if (obj_a->type != obj_b->type)
                return XEN_FALSE;

            /* Compare based on object type */
            switch (obj_a->type) {
                case OBJ_STRING: {
                    xen_obj_str* str_a = (xen_obj_str*)obj_a;
                    xen_obj_str* str_b = (xen_obj_str*)obj_b;
                    /* With string interning, same hash + same length + same pointer should suffice,
                       but let's be thorough in case strings bypass interning */
                    if (str_a->length != str_b->length)
                        return XEN_FALSE;
                    if (str_a->hash != str_b->hash)
                        return XEN_FALSE;
                    return memcmp(str_a->str, str_b->str, str_a->length) == 0;
                }
                case OBJ_ARRAY: {
                    xen_obj_array* arr_a = (xen_obj_array*)obj_a;
                    xen_obj_array* arr_b = (xen_obj_array*)obj_b;
                    if (arr_a->array.count != arr_b->array.count)
                        return XEN_FALSE;
                    for (i32 i = 0; i < arr_a->array.count; i++) {
                        if (!xen_value_equal(arr_a->array.values[i], arr_b->array.values[i]))
                            return XEN_FALSE;
                    }
                    return XEN_TRUE;
                }
                default:
                    /* Functions, namespaces, etc. - use pointer equality */
                    return XEN_FALSE;
            }
        }
        case VAL_NULL:
            return XEN_TRUE;
        default:
            return XEN_FALSE;
    }
}

void xen_value_array_init(xen_value_array* array) {
    array->count  = 0;
    array->cap    = 0;
    array->values = NULL;
}

void xen_value_array_write(xen_value_array* array, xen_value value) {
    if (array->cap < array->count + 1) {
        const u64 old_cap = array->cap;
        array->cap        = XEN_GROW_CAPACITY(old_cap);
        array->values     = XEN_GROW_ARRAY(xen_value, array->values, old_cap, array->cap);
    }

    array->values[array->count] = value;
    array->count++;
}

void xen_value_array_free(xen_value_array* array) {
    XEN_FREE_ARRAY(xen_value, array->values, array->cap);
    xen_value_array_init(array);
}

void xen_value_print(xen_value value) {
    switch (value.type) {
        case VAL_BOOL:
            printf(VAL_AS_BOOL(value) ? "true" : "false");
            break;
        case VAL_NULL:
            printf("null");
            break;
        case VAL_NUMBER:
            printf("%g", VAL_AS_NUMBER(value));
            break;
        case VAL_OBJECT: {
            xen_obj_print(value);
        } break;
        default:
            return;  // Unreachable
    }
}
