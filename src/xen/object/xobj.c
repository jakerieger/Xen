#include "xobj.h"
#include "xobj_string.h"
#include "xobj_function.h"
#include "xobj_native_function.h"
#include "xobj_namespace.h"
#include "xobj_array.h"
#include "xobj_bound_method.h"
#include "xobj_dict.h"
#include "xobj_class.h"
#include "xobj_instance.h"
#include "xobj_u8array.h"
#include "xobj_error.h"
#include "../xutils.h"
#include "../xvm.h"

bool xen_obj_is_type(xen_value value, xen_obj_type type) {
    return VAL_IS_OBJ(value) && VAL_AS_OBJ(value)->type == type;
}

static void print_function(xen_obj_func* fn) {
    if (fn->name == NULL) {
        printf("<Script>");
        return;
    }
    printf("<Function %s>", fn->name->str);
}

void xen_obj_print(xen_value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING: {
            const char* val = OBJ_AS_CSTRING(value);
            const char* dec = xen_decode_string_literal(val, strlen(val), NULL);
            printf("%s", dec);
        } break;
        case OBJ_FUNCTION: {
            print_function(OBJ_AS_FUNCTION(value));
        } break;
        case OBJ_NATIVE_FUNC: {
            printf("<Function ::%s>", OBJ_AS_NATIVE_FUNC(value)->name);
        } break;
        case OBJ_NAMESPACE: {
            printf("<Namespace %s>", OBJ_AS_NAMESPACE(value)->name);
            break;
        }
        case OBJ_ARRAY: {
            printf("[ ");
            xen_obj_array* array = OBJ_AS_ARRAY(value);
            for (int i = 0; i < array->array.count; i++) {
                xen_value val = array->array.values[i];
                xen_value_print(val);
                if (i < array->array.count - 1) {
                    printf(", ");
                }
            }
            printf(" ]");
            break;
        }
        case OBJ_BOUND_METHOD: {
            printf("<BoundMethod %s>", OBJ_AS_BOUND_METHOD(value)->name);
            break;
        }
        case OBJ_DICT: {
            printf("{ ");
            xen_obj_dict* dict = OBJ_AS_DICT(value);
            bool first         = XEN_TRUE;
            for (i32 i = 0; i < dict->table.capacity; i++) {
                xen_table_entry* entry = &dict->table.entries[i];
                if (entry->key != NULL) {
                    if (!first)
                        printf(", ");
                    printf("\"%s\": ", entry->key->str);
                    xen_value_print(entry->value);
                    first = XEN_FALSE;
                }
            }
            printf(" }");
            break;
        }
        case OBJ_CLASS: {
            printf("<Class %s>", OBJ_AS_CLASS(value)->name->str);
            break;
        }
        case OBJ_INSTANCE: {
            xen_obj_instance* inst = OBJ_AS_INSTANCE(value);
            printf("<%s : instance>", inst->class->name->str);
            break;
        }
        case OBJ_U8ARRAY: {
            printf("(UInt8Array)[ ");
            xen_obj_u8array* array = OBJ_AS_U8ARRAY(value);
            for (int i = 0; i < array->count; i++) {
                u8 val = array->values[i];
                printf("%02hhX", val);
                if (i < array->count - 1) {
                    printf(", ");
                }
            }
            printf(" ]");
            break;
        }
        case OBJ_ERROR: {
            printf("<Error: '%s'>", OBJ_ERROR_GET_MSG_CSTR(value));
            break;
        }
    }
}

xen_obj* xen_obj_allocate(size_t size, xen_obj_type type) {
    xen_obj* obj = (xen_obj*)xen_mem_realloc(NULL, 0, size);
    obj->type    = type;
    obj->next    = g_vm.objects;
    g_vm.objects = obj;
    return obj;
}

static xen_native_fn lookup_in_table(xen_method_entry* table, const char* name, bool* is_property) {
    for (i32 i = 0; table[i].name != NULL; i++) {
        if (strcmp(table[i].name, name) == 0) {
            if (is_property)
                *is_property = table[i].is_property;
            return table[i].method;
        }
    }
    return NULL;
}

xen_native_fn xen_lookup_method(xen_value value, const char* name, bool* is_property) {
    if (is_property)
        *is_property = XEN_FALSE;

    if (!VAL_IS_OBJ(value)) {
        return NULL;
    }

    switch (OBJ_TYPE(value)) {
        case OBJ_STRING:
            return lookup_in_table(k_string_methods, name, is_property);
        case OBJ_ARRAY:
            return lookup_in_table(k_array_methods, name, is_property);
        case OBJ_DICT:
            return lookup_in_table(k_dict_methods, name, is_property);
        case OBJ_ERROR:
            return lookup_in_table(k_error_methods, name, is_property);
        default:
            return NULL;
    }
}
