#include "xbuiltin_array.h"
#include "xbuiltin_common.h"

xen_value xen_arr_len(i32 argc, xen_value* argv) {
    if (argc < 1 || !OBJ_IS_ARRAY(argv[0]))
        return NUMBER_VAL(-1);
    return NUMBER_VAL(OBJ_AS_ARRAY(argv[0])->array.count);
}

xen_value xen_arr_push(i32 argc, xen_value* argv) {
    if (argc < 2 || !OBJ_IS_ARRAY(argv[0]))
        return NULL_VAL;
    xen_obj_array* arr = OBJ_AS_ARRAY(argv[0]);
    for (i32 i = 1; i < argc; i++) {
        xen_obj_array_push(arr, argv[i]);
    }
    return NUMBER_VAL(arr->array.count);
}

xen_value xen_arr_pop(i32 argc, xen_value* argv) {
    if (argc < 1 || !OBJ_IS_ARRAY(argv[0]))
        return NULL_VAL;
    return xen_obj_array_pop(OBJ_AS_ARRAY(argv[0]));
}

xen_value xen_arr_first(i32 argc, xen_value* argv) {
    if (argc < 1 || !OBJ_IS_ARRAY(argv[0]))
        return NULL_VAL;
    xen_obj_array* arr = OBJ_AS_ARRAY(argv[0]);
    if (arr->array.count == 0)
        return NULL_VAL;
    return arr->array.values[0];
}

xen_value xen_arr_last(i32 argc, xen_value* argv) {
    if (argc < 1 || !OBJ_IS_ARRAY(argv[0]))
        return NULL_VAL;
    xen_obj_array* arr = OBJ_AS_ARRAY(argv[0]);
    if (arr->array.count == 0)
        return NULL_VAL;
    return arr->array.values[arr->array.count - 1];
}

xen_value xen_arr_clear(i32 argc, xen_value* argv) {
    if (argc < 1 || !OBJ_IS_ARRAY(argv[0]))
        return NULL_VAL;
    OBJ_AS_ARRAY(argv[0])->array.count = 0;
    return NULL_VAL;
}

xen_value xen_arr_contains(i32 argc, xen_value* argv) {
    if (argc < 2 || !OBJ_IS_ARRAY(argv[0]))
        return BOOL_VAL(XEN_FALSE);
    xen_obj_array* arr = OBJ_AS_ARRAY(argv[0]);
    xen_value needle   = argv[1];
    for (i32 i = 0; i < arr->array.count; i++) {
        if (xen_value_equal(arr->array.values[i], needle)) {
            return BOOL_VAL(XEN_TRUE);
        }
    }
    return BOOL_VAL(XEN_FALSE);
}

xen_value xen_arr_index_of(i32 argc, xen_value* argv) {
    if (argc < 2 || !OBJ_IS_ARRAY(argv[0]))
        return NUMBER_VAL(-1);
    xen_obj_array* arr = OBJ_AS_ARRAY(argv[0]);
    xen_value needle   = argv[1];
    for (i32 i = 0; i < arr->array.count; i++) {
        if (xen_value_equal(arr->array.values[i], needle)) {
            return NUMBER_VAL(i);
        }
    }
    return NUMBER_VAL(-1);
}

xen_value xen_arr_reverse(i32 argc, xen_value* argv) {
    if (argc < 1 || !OBJ_IS_ARRAY(argv[0]))
        return NULL_VAL;
    xen_obj_array* arr = OBJ_AS_ARRAY(argv[0]);
    for (i32 i = 0; i < arr->array.count / 2; i++) {
        i32 j                = arr->array.count - 1 - i;
        xen_value temp       = arr->array.values[i];
        arr->array.values[i] = arr->array.values[j];
        arr->array.values[j] = temp;
    }
    return argv[0];
}

xen_value xen_arr_join(i32 argc, xen_value* argv) {
    if (argc < 1 || !OBJ_IS_ARRAY(argv[0]))
        return NULL_VAL;

    xen_obj_array* arr = OBJ_AS_ARRAY(argv[0]);
    char* delim        = ", ";

    if (argc > 1 && VAL_IS_OBJ(argv[1]) && OBJ_IS_STRING(argv[1])) {
        delim = OBJ_AS_CSTRING(argv[1]);
    }

    const i32 size                     = arr->array.count;
    XEN_CLEANUP_FREE xen_value* values = (xen_value*)malloc(sizeof(xen_value) * size);
    size_t strbuf_size_needed          = 0;

    for (i32 i = 0; i < size; i++) {
        xen_value arg[] = {arr->array.values[i]};
        // use string constructor to handle automatically converting each array value to a string
        xen_value as_str = xen_builtin_string_ctor(1, arg);
        values[i]        = as_str;
        strbuf_size_needed += OBJ_AS_STRING(as_str)->length;
        if (i < size - 1) {
            strbuf_size_needed += strlen(delim);
        }
    }

    XEN_CLEANUP_FREE char* strbuf = (char*)malloc(strbuf_size_needed + 1);
    size_t offset                 = 0;
    for (i32 i = 0; i < size; i++) {
        xen_obj_str* str = OBJ_AS_STRING(values[i]);
        memcpy(strbuf + offset, str->str, str->length);
        offset += str->length;

        if (i < size - 1) {
            memcpy(strbuf + offset, delim, strlen(delim));
            offset += strlen(delim);
        }
    }
    strbuf[strbuf_size_needed] = '\0';

    return OBJ_VAL(xen_obj_str_copy(strbuf, strbuf_size_needed));
}

xen_obj_namespace* xen_builtin_array() {
    xen_obj_namespace* arr = xen_obj_namespace_new("array");
    xen_obj_namespace_set(arr, "len", OBJ_VAL(xen_obj_native_func_new(xen_arr_len, "len")));
    xen_obj_namespace_set(arr, "push", OBJ_VAL(xen_obj_native_func_new(xen_arr_push, "push")));
    xen_obj_namespace_set(arr, "pop", OBJ_VAL(xen_obj_native_func_new(xen_arr_pop, "pop")));
    xen_obj_namespace_set(arr, "first", OBJ_VAL(xen_obj_native_func_new(xen_arr_first, "first")));
    xen_obj_namespace_set(arr, "last", OBJ_VAL(xen_obj_native_func_new(xen_arr_last, "last")));
    xen_obj_namespace_set(arr, "clear", OBJ_VAL(xen_obj_native_func_new(xen_arr_clear, "clear")));
    xen_obj_namespace_set(arr, "contains", OBJ_VAL(xen_obj_native_func_new(xen_arr_contains, "contains")));
    xen_obj_namespace_set(arr, "index_of", OBJ_VAL(xen_obj_native_func_new(xen_arr_index_of, "index_of")));
    xen_obj_namespace_set(arr, "reverse", OBJ_VAL(xen_obj_native_func_new(xen_arr_reverse, "reverse")));
    xen_obj_namespace_set(arr, "join", OBJ_VAL(xen_obj_native_func_new(xen_arr_join, "join")));
    return arr;
}