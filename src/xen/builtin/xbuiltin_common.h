#ifndef X_BUILTIN_COMMON_H
#define X_BUILTIN_COMMON_H

#include "../xobject.h"
#include "../xtable.h"
#include "../xvm.h"
#include "../xtypeid.h"

#define REQUIRE_ARG(name, slot, typeid)                                                                                \
    do {                                                                                                               \
        if (argc < slot + 1) {                                                                                         \
            xen_runtime_error("argument '%s' (position %d) required for %s", name, slot, __func__);                    \
            return NULL_VAL;                                                                                           \
        }                                                                                                              \
        if (typeid != xen_typeid_get(argv[slot]) && typeid != TYPEID_UNDEFINED) {                                      \
            xen_runtime_error("expected typeid %d for argment '%s' (got %d"), typeid, name,                            \
              xen_typeid_get(argv[slot]);                                                                              \
            return NULL_VAL;                                                                                           \
        }                                                                                                              \
    } while (XEN_FALSE)

inline static void define_native_fn(const char* name, xen_native_fn fn) {
    xen_obj_str* key = xen_obj_str_copy(name, strlen(name));
    xen_table_set(&g_vm.globals, key, OBJ_VAL(xen_obj_native_func_new(fn, name)));
}

// type constructors
inline static xen_value xen_builtin_number_ctor(i32 argc, xen_value* argv) {
    REQUIRE_ARG("construct_from", 0, TYPEID_UNDEFINED);
    xen_value val = argv[0];
    switch (val.type) {
        case VAL_BOOL:
            return NUMBER_VAL(VAL_AS_BOOL(val));
        case VAL_NULL:
            return NUMBER_VAL(0);
        case VAL_NUMBER:
            return val;
        case VAL_OBJECT: {
            if (OBJ_IS_STRING(val)) {
                const char* str = OBJ_AS_CSTRING(val);
                f64 num         = strtod(str, NULL);
                return NUMBER_VAL(num);
            } else {
                const char* type_str = xen_value_type_to_str(val);
                xen_runtime_error("cannot construct number from %s", type_str);
                return NULL_VAL;
            }
        }
    }

    return NULL_VAL;
}

inline static xen_value xen_builtin_string_ctor(i32 argc, xen_value* argv) {
    REQUIRE_ARG("construct_from", 0, TYPEID_UNDEFINED);
    xen_value val = argv[0];
    switch (val.type) {
        case VAL_BOOL: {
            const char* bool_str = VAL_AS_BOOL(val) == XEN_TRUE ? "true" : "false";
            return OBJ_VAL(xen_obj_str_copy(bool_str, strlen(bool_str)));
        }
        case VAL_NULL:
            return OBJ_VAL(xen_obj_str_copy("null", 4));
        case VAL_NUMBER: {
            char buffer[128] = {'\0'};
            snprintf(buffer, sizeof(buffer), "%f", VAL_AS_NUMBER(val));
            return OBJ_VAL(xen_obj_str_copy(buffer, strlen(buffer)));
        }
        case VAL_OBJECT: {
            if (OBJ_IS_STRING(val)) {
                return val;
            }
            break;
        }
    }

    const char* obj_str = xen_value_type_to_str(val);
    return OBJ_VAL(xen_obj_str_copy(obj_str, strlen(obj_str)));
}

inline static xen_value xen_builtin_bool_ctor(i32 argc, xen_value* argv) {
    REQUIRE_ARG("construct_from", 0, TYPEID_UNDEFINED);
    xen_value val = argv[0];
    switch (val.type) {
        case VAL_BOOL: {
            return val;
        }
        case VAL_NULL:
            return BOOL_VAL(XEN_FALSE);
        case VAL_NUMBER: {
            if (VAL_AS_NUMBER(val) != 0) {
                return BOOL_VAL(XEN_TRUE);
            } else {
                return BOOL_VAL(XEN_FALSE);
            }
        }
        default:
            break;
    }

    return BOOL_VAL(XEN_TRUE);
}

// signature: (number of elements, default value (or null if missing))
inline static xen_value xen_builtin_array_ctor(i32 argc, xen_value* argv) {
    if (argc > 2) {
        xen_runtime_error("array constructor has invalid number of arguments");
        return NULL_VAL;
    }

    if (argc > 0 && argv[0].type != VAL_NUMBER) {
        xen_runtime_error("element count must be a number");
        return NULL_VAL;
    }

    i32 element_count       = (argc > 0 && VAL_IS_NUMBER(argv[0])) ? VAL_AS_NUMBER(argv[0]) : 0;
    bool has_default        = (argc == 2) ? XEN_TRUE : XEN_FALSE;
    xen_value default_value = (has_default) ? argv[1] : NULL_VAL;

    // create the array
    xen_obj_array* arr = xen_obj_array_new_with_capacity(element_count);
    for (i32 i = 0; i < element_count; i++) {
        xen_obj_array_push(arr, default_value);
    }

    return OBJ_VAL(arr);
}

#endif