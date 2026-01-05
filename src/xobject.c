#include "xobject.h"
#include "xalloc.h"
#include "xtable.h"
#include "xutils.h"
#include "xvalue.h"
#include "xvm.h"
#include "xbuiltin.h"

#define ALLOCATE_OBJ(type, obj_type) (type*)allocate_obj(sizeof(type), obj_type);

bool xen_obj_is_type(xen_value value, xen_obj_type type) {
    return VAL_IS_OBJ(value) && VAL_AS_OBJ(value)->type == type;
}

static void print_function(xen_obj_func* fn) {
    if (fn->name == NULL) {
        printf("<script>");
        return;
    }
    printf("<Function %s>", fn->name->str);
}

void xen_obj_print(xen_value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING: {
            const char* val = OBJ_AS_CSTRING(value);
            printf("%s", val);
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
    }
}

static xen_obj* allocate_obj(size_t size, xen_obj_type type) {
    xen_obj* obj = (xen_obj*)xen_mem_realloc(NULL, 0, size);
    obj->type    = type;
    obj->next    = g_vm.objects;
    g_vm.objects = obj;
    return obj;
}

static xen_obj_str* allocate_str(char* chars, i32 length, u32 hash) {
    xen_obj_str* str = ALLOCATE_OBJ(xen_obj_str, OBJ_STRING);
    str->length      = length;
    str->str         = chars;
    str->hash        = hash;
    xen_table_set(&g_vm.strings, str, NULL_VAL);
    return str;
}

xen_obj_str* xen_obj_str_take(char* chars, i32 length) {
    u32 hash              = xen_hash_string(chars, length);
    xen_obj_str* interned = xen_table_find_str(&g_vm.strings, chars, length, hash);
    if (interned != NULL) {
        XEN_FREE_ARRAY(char, chars, length + 1);
        return interned;
    }
    return allocate_str(chars, length, hash);
}

xen_obj_str* xen_obj_str_copy(const char* chars, i32 length) {
    u32 hash              = xen_hash_string(chars, length);
    xen_obj_str* interned = xen_table_find_str(&g_vm.strings, chars, length, hash);
    if (interned != NULL)
        return interned;

    char* heap_chars = XEN_ALLOCATE(char, length + 1);
    if (!heap_chars) {
        xen_panic(XEN_ERR_ALLOCATION_FAILED, "failed to allocate heap memory for string");
    }

    memcpy(heap_chars, chars, length);
    heap_chars[length] = '\0';

    return allocate_str(heap_chars, length, hash);
}

xen_obj_func* xen_obj_func_new() {
    xen_obj_func* fn = ALLOCATE_OBJ(xen_obj_func, OBJ_FUNCTION);
    fn->arity        = 0;
    fn->name         = NULL;
    xen_chunk_init(&fn->chunk);
    return fn;
}

xen_obj_native_func* xen_obj_native_func_new(xen_native_fn function, const char* name) {
    xen_obj_native_func* fn = ALLOCATE_OBJ(xen_obj_native_func, OBJ_NATIVE_FUNC);
    fn->function            = function;
    fn->name                = name;
    return fn;
}

xen_obj_namespace* xen_obj_namespace_new(const char* name) {
    xen_obj_namespace* ns = ALLOCATE_OBJ(xen_obj_namespace, OBJ_NAMESPACE);
    ns->name              = name;
    ns->entries           = NULL;
    ns->count             = 0;
    ns->capacity          = 0;
    return ns;
}

void xen_obj_namespace_set(xen_obj_namespace* ns, const char* name, xen_value value) {
    for (i32 i = 0; i < ns->count; i++) {
        if (strcmp(ns->entries[i].name, name) == 0) {
            ns->entries[i].value = value;
            return;
        }
    }

    if (ns->count >= ns->capacity) {
        i32 old_cap  = ns->capacity;
        ns->capacity = XEN_GROW_CAPACITY(old_cap);
        ns->entries  = XEN_GROW_ARRAY(xen_ns_entry, ns->entries, old_cap, ns->capacity);
    }

    ns->entries[ns->count].name  = name;
    ns->entries[ns->count].value = value;
    ns->count++;
}

bool xen_obj_namespace_get(xen_obj_namespace* ns, const char* name, xen_value* out) {
    for (i32 i = 0; i < ns->count; i++) {
        if (strcmp(ns->entries[i].name, name) == 0) {
            *out = ns->entries[i].value;
            return XEN_TRUE;
        }
    }
    return XEN_FALSE;
}

xen_obj_array* xen_obj_array_new() {
    return xen_obj_array_new_with_capacity(8);
}

xen_obj_array* xen_obj_array_new_with_capacity(i32 capacity) {
    xen_obj_array* arr = ALLOCATE_OBJ(xen_obj_array, OBJ_ARRAY);
    xen_value_array_init(&arr->array);
    if (capacity > 0) {
        arr->array.values = XEN_ALLOCATE(xen_value, capacity);
    }
    return arr;
}

void xen_obj_array_push(xen_obj_array* arr, xen_value value) {
    xen_value_array_write(&arr->array, value);
}

xen_value xen_obj_array_get(xen_obj_array* arr, i32 index) {
    if (index < 0 || index >= arr->array.count) {
        return NULL_VAL;
    }
    return arr->array.values[index];
}

void xen_obj_array_set(xen_obj_array* arr, i32 index, xen_value value) {
    if (index < 0 || index >= arr->array.count) {
        return;  // out of bounds - silently fail for now
    }
    arr->array.values[index] = value;
}

xen_value xen_obj_array_pop(xen_obj_array* arr) {
    if (arr->array.count == 0)
        return NULL_VAL;
    arr->array.count--;
    return arr->array.values[arr->array.count];
}

i32 xen_obj_array_length(xen_obj_array* arr) {
    return arr->array.count;
}

xen_obj_bound_method* xen_obj_bound_method_new(xen_value receiver, xen_native_fn method, const char* name) {
    xen_obj_bound_method* bound = ALLOCATE_OBJ(xen_obj_bound_method, OBJ_BOUND_METHOD);
    bound->receiver             = receiver;
    bound->method               = method;
    bound->name                 = name;
    bound->function             = NULL;
    return bound;
}

xen_obj_bound_method* xen_obj_bound_method_new_func(xen_value receiver, xen_obj_func* func) {
    xen_obj_bound_method* bound = ALLOCATE_OBJ(xen_obj_bound_method, OBJ_BOUND_METHOD);
    bound->receiver             = receiver;
    bound->method               = NULL;
    bound->function             = func;
    bound->name                 = func->name ? func->name->str : "<anonymous>";
    return bound;
}

/* ============================================================================
 * method tables
 * ============================================================================ */

// clang-format off
static xen_method_entry string_methods[] = {
     {"len",         xen_str_len,         XEN_TRUE},   // property
     {"upper",       xen_str_upper,       XEN_FALSE},
     {"lower",       xen_str_lower,       XEN_FALSE},
     {"trim",        xen_str_trim,        XEN_FALSE},
     {"contains",    xen_str_contains,    XEN_FALSE},
     {"starts_with", xen_str_starts_with, XEN_FALSE},
     {"ends_with",   xen_str_ends_with,   XEN_FALSE},
     {"substr",      xen_str_substr,      XEN_FALSE},
     {"find",        xen_str_find,        XEN_FALSE},
     {"split",       xen_str_split,       XEN_FALSE},
    {"replace",     xen_str_replace,     XEN_FALSE},
    {NULL,          NULL,                XEN_FALSE}
};

static xen_method_entry array_methods[] = {
     {"len",       xen_arr_len,       XEN_TRUE},   // property
     {"push",      xen_arr_push,      XEN_FALSE},
     {"pop",       xen_arr_pop,       XEN_FALSE},
     {"first",     xen_arr_first,     XEN_TRUE},   // property
     {"last",      xen_arr_last,      XEN_TRUE},   // property
     {"clear",     xen_arr_clear,     XEN_FALSE},
     {"contains",  xen_arr_contains,  XEN_FALSE},
     {"index_of",  xen_arr_index_of,  XEN_FALSE},
     {"reverse",   xen_arr_reverse,   XEN_FALSE},
     {"join",      xen_arr_join,      XEN_FALSE},
    {NULL,        NULL,              XEN_FALSE}
};

static xen_method_entry number_methods[] = {
     {"abs",       xen_num_abs,       XEN_FALSE},
     {"floor",     xen_num_floor,     XEN_FALSE},
     {"ceil",      xen_num_ceil,      XEN_FALSE},
     {"round",     xen_num_round,     XEN_FALSE},
     {"to_string", xen_num_to_string, XEN_FALSE},
     {NULL,        NULL,              XEN_FALSE}
};

static xen_method_entry dict_methods[] = {
     {"len",       xen_dict_len,      XEN_TRUE},   // property
     {"keys",      xen_dict_keys,     XEN_FALSE},
     {"values",    xen_dict_values,   XEN_FALSE},
     {"has",       xen_dict_has,      XEN_FALSE},
     {"remove",    xen_dict_remove,   XEN_FALSE},
     {"clear",     xen_dict_clear,    XEN_FALSE},
     {NULL,        NULL,              XEN_FALSE}
};
// clang-format on

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

    if (VAL_IS_NUMBER(value)) {
        return lookup_in_table(number_methods, name, is_property);
    }

    if (!VAL_IS_OBJ(value)) {
        return NULL;
    }

    switch (OBJ_TYPE(value)) {
        case OBJ_STRING:
            return lookup_in_table(string_methods, name, is_property);
        case OBJ_ARRAY:
            return lookup_in_table(array_methods, name, is_property);
        case OBJ_DICT:
            return lookup_in_table(dict_methods, name, is_property);
        default:
            return NULL;
    }
}

xen_obj_dict* xen_obj_dict_new() {
    xen_obj_dict* dict = ALLOCATE_OBJ(xen_obj_dict, OBJ_DICT);
    xen_table_init(&dict->table);
    return dict;
}

void xen_obj_dict_set(xen_obj_dict* dict, xen_value key, xen_value value) {
    // For now, only allow string keys (simplest approach)
    if (!OBJ_IS_STRING(key)) {
        xen_runtime_error("dictionary keys must be strings");
        return;
    }
    xen_obj_str* key_str = OBJ_AS_STRING(key);
    xen_table_set(&dict->table, key_str, value);
}

bool xen_obj_dict_get(xen_obj_dict* dict, xen_value key, xen_value* out) {
    if (!OBJ_IS_STRING(key)) {
        return XEN_FALSE;
    }
    xen_obj_str* key_str = OBJ_AS_STRING(key);
    return xen_table_get(&dict->table, key_str, out);
}

bool xen_obj_dict_delete(xen_obj_dict* dict, xen_value key) {
    if (!OBJ_IS_STRING(key)) {
        return XEN_FALSE;
    }
    xen_obj_str* key_str = OBJ_AS_STRING(key);
    return xen_table_delete(&dict->table, key_str);
}

xen_obj_class* xen_obj_class_new(xen_obj_str* name) {
    xen_obj_class* class     = ALLOCATE_OBJ(xen_obj_class, OBJ_CLASS);
    class->name              = name;
    class->properties        = NULL;
    class->property_count    = 0;
    class->property_capacity = 0;
    xen_table_init(&class->methods);
    xen_table_init(&class->private_methods);
    class->initializer = NULL;
    return class;
}

void xen_obj_class_add_property(xen_obj_class* class, xen_obj_str* name, xen_value default_val, bool is_private) {
    if (class->property_count >= class->property_capacity) {
        i32 old_cap              = class->property_capacity;
        class->property_capacity = XEN_GROW_CAPACITY(old_cap);
        class->properties = XEN_GROW_ARRAY(xen_property_def, class->properties, old_cap, class->property_capacity);
    }

    xen_property_def* prop = &class->properties[class->property_count];
    prop->name             = name;
    prop->default_value    = default_val;
    prop->is_private       = is_private;
    prop->index            = class->property_count;
    class->property_count++;
}

void xen_obj_class_add_method(xen_obj_class* class, xen_obj_str* name, xen_obj_func* method, bool is_private) {
    xen_table* table = is_private ? &class->private_methods : &class->methods;
    xen_table_set(table, name, OBJ_VAL(method));
}

xen_obj_instance* xen_obj_instance_new(xen_obj_class* class) {
    xen_obj_instance* instance = ALLOCATE_OBJ(xen_obj_instance, OBJ_INSTANCE);
    instance->class            = class;

    // Allocate fields and set defaults
    instance->fields = XEN_ALLOCATE(xen_value, class->property_count);
    for (i32 i = 0; i < class->property_count; i++) {
        instance->fields[i] = class->properties[i].default_value;
    }

    return instance;
}

// Find property index by name
static i32 find_property_index(xen_obj_class* class, xen_obj_str* name) {
    for (i32 i = 0; i < class->property_count; i++) {
        if (class->properties[i].name == name ||
            (class->properties[i].name->length == name->length &&
             memcmp(class->properties[i].name->str, name->str, name->length) == 0)) {
            return i;
        }
    }
    return -1;
}

bool xen_obj_instance_get(xen_obj_instance* inst, xen_obj_str* name, xen_value* out) {
    i32 index = find_property_index(inst->class, name);
    if (index >= 0) {
        *out = inst->fields[index];
        return XEN_TRUE;
    }
    return XEN_FALSE;
}

bool xen_obj_instance_set(xen_obj_instance* inst, xen_obj_str* name, xen_value value) {
    i32 index = find_property_index(inst->class, name);
    if (index >= 0) {
        inst->fields[index] = value;
        return XEN_TRUE;
    }
    return XEN_FALSE;
}
