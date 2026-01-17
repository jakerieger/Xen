#include "xvalue.h"
#include "xmem.h"
#include "math.h"

#include "object/xobj_string.h"
#include "object/xobj_array.h"
#include "object/xobj_dict.h"
#include "object/xobj_function.h"
#include "object/xobj_native_function.h"
#include "object/xobj_namespace.h"
#include "object/xobj_bound_method.h"

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

            if (obj_a == obj_b)
                return XEN_TRUE;

            if (obj_a->type != obj_b->type)
                return XEN_FALSE;

            switch (obj_a->type) {
                case OBJ_STRING: {
                    xen_obj_str* str_a = (xen_obj_str*)obj_a;
                    xen_obj_str* str_b = (xen_obj_str*)obj_b;
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
                case OBJ_DICT: {
                    xen_obj_dict* dict_a = (xen_obj_dict*)obj_a;
                    xen_obj_dict* dict_b = (xen_obj_dict*)obj_b;
                    if (dict_a->table.count != dict_b->table.count)
                        return XEN_FALSE;
                    for (i32 i = 0; i < dict_a->table.count; i++) {
                        if (strcmp(dict_a->table.entries[i].key->str, dict_b->table.entries[i].key->str) != 0)
                            return XEN_FALSE;

                        if (!xen_value_equal(dict_a->table.entries[i].value, dict_b->table.entries[i].value))
                            return XEN_FALSE;
                    }
                    return XEN_TRUE;
                }
                default:
                    // Functions, namespaces, etc. - use pointer equality
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
        case VAL_NUMBER: {
            f64 num = VAL_AS_NUMBER(value);
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "%.17g", num);
            if (strchr(buffer, 'e') && fabs(num) < 1.0 && fabs(num) > 1e-10) {
                snprintf(buffer, sizeof(buffer), "%.17f", num);
                char* end = buffer + strlen(buffer) - 1;
                while (end > buffer && *end == '0')
                    *end-- = '\0';
                if (end > buffer && *end == '.')
                    *end = '\0';
            }
            printf("%s", buffer);
            break;
        }
        case VAL_OBJECT: {
            xen_obj_print(value);
        } break;
        default:
            return;  // Unreachable
    }
}

// TODO: I don't think I need this anymore. I'm not sure where it's used but I'll remove it when I find it.
// typeid handles this as well as some other stuff so it's redundant at best.
char* xen_value_type_to_str(xen_value value) {
    switch (value.type) {
        case VAL_BOOL:
            return "bool";
        case VAL_NULL:
            return "null";
        case VAL_NUMBER:
            return "number";
        case VAL_OBJECT: {
            if (OBJ_IS_STRING(value)) {
                return "string";
            } else if (OBJ_IS_FUNCTION(value)) {
                return "function";
            } else if (OBJ_IS_NATIVE_FUNC(value)) {
                return "native_function";
            } else if (OBJ_IS_NAMESPACE(value)) {
                return "namespace";
            } else if (OBJ_IS_ARRAY(value)) {
                return "array";
            } else if (OBJ_IS_BOUND_METHOD(value)) {
                return "bound_method";
            }
        }
    }
    return NULL;
}
