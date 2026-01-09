#include "xbuiltin_dict.h"
#include "xbuiltin_common.h"

xen_value xen_dict_len(i32 argc, xen_value* argv) {
    if (argc < 1 || !OBJ_IS_DICT(argv[0]))
        return NUMBER_VAL(0);
    xen_obj_dict* dict = OBJ_AS_DICT(argv[0]);
    return NUMBER_VAL(dict->table.count);
}

xen_value xen_dict_keys(i32 argc, xen_value* argv) {
    if (argc < 1 || !OBJ_IS_DICT(argv[0]))
        return NULL_VAL;

    xen_obj_dict* dict  = OBJ_AS_DICT(argv[0]);
    xen_obj_array* keys = xen_obj_array_new();

    for (i32 i = 0; i < dict->table.capacity; i++) {
        xen_table_entry* entry = &dict->table.entries[i];
        if (entry->key != NULL) {
            xen_obj_array_push(keys, OBJ_VAL(entry->key));
        }
    }

    return OBJ_VAL(keys);
}

xen_value xen_dict_values(i32 argc, xen_value* argv) {
    if (argc < 1 || !OBJ_IS_DICT(argv[0]))
        return NULL_VAL;

    xen_obj_dict* dict    = OBJ_AS_DICT(argv[0]);
    xen_obj_array* values = xen_obj_array_new();

    for (i32 i = 0; i < dict->table.capacity; i++) {
        xen_table_entry* entry = &dict->table.entries[i];
        if (entry->key != NULL) {
            xen_obj_array_push(values, entry->value);
        }
    }

    return OBJ_VAL(values);
}

xen_value xen_dict_has(i32 argc, xen_value* argv) {
    if (argc < 2 || !OBJ_IS_DICT(argv[0]))
        return BOOL_VAL(XEN_FALSE);

    xen_obj_dict* dict = OBJ_AS_DICT(argv[0]);
    xen_value dummy;
    return BOOL_VAL(xen_obj_dict_get(dict, argv[1], &dummy));
}

xen_value xen_dict_remove(i32 argc, xen_value* argv) {
    if (argc < 2 || !OBJ_IS_DICT(argv[0]))
        return BOOL_VAL(XEN_FALSE);

    xen_obj_dict* dict = OBJ_AS_DICT(argv[0]);
    return BOOL_VAL(xen_obj_dict_delete(dict, argv[1]));
}

xen_value xen_dict_clear(i32 argc, xen_value* argv) {
    if (argc < 1 || !OBJ_IS_DICT(argv[0]))
        return NULL_VAL;

    xen_obj_dict* dict = OBJ_AS_DICT(argv[0]);
    xen_table_free(&dict->table);
    xen_table_init(&dict->table);
    return NULL_VAL;
}

xen_obj_namespace* xen_builtin_dict() {
    xen_obj_namespace* dict = xen_obj_namespace_new("dict");
    xen_obj_namespace_set(dict, "len", OBJ_VAL(xen_obj_native_func_new(xen_dict_len, "len")));
    xen_obj_namespace_set(dict, "keys", OBJ_VAL(xen_obj_native_func_new(xen_dict_keys, "keys")));
    xen_obj_namespace_set(dict, "values", OBJ_VAL(xen_obj_native_func_new(xen_dict_values, "values")));
    xen_obj_namespace_set(dict, "has", OBJ_VAL(xen_obj_native_func_new(xen_dict_has, "has")));
    xen_obj_namespace_set(dict, "remove", OBJ_VAL(xen_obj_native_func_new(xen_dict_remove, "remove")));
    xen_obj_namespace_set(dict, "clear", OBJ_VAL(xen_obj_native_func_new(xen_dict_clear, "clear")));
    return dict;
}