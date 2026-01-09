#include "xbuiltin_string.h"
#include "xbuiltin_common.h"
#include <ctype.h>

xen_value xen_str_len(i32 argc, xen_value* argv) {
    if (argc < 1 || !OBJ_IS_STRING(argv[0]))
        return NULL_VAL;
    return NUMBER_VAL(OBJ_AS_STRING(argv[0])->length);
}

xen_value xen_str_upper(i32 argc, xen_value* argv) {
    if (argc < 1 || !OBJ_IS_STRING(argv[0]))
        return NULL_VAL;
    xen_obj_str* str = OBJ_AS_STRING(argv[0]);
    char* buffer     = malloc(str->length + 1);
    for (i32 i = 0; i < str->length; i++) {
        buffer[i] = toupper(str->str[i]);
    }
    buffer[str->length] = '\0';
    return OBJ_VAL(xen_obj_str_take(buffer, str->length));
}

xen_value xen_str_lower(i32 argc, xen_value* argv) {
    if (argc < 1 || !OBJ_IS_STRING(argv[0]))
        return NULL_VAL;
    xen_obj_str* str = OBJ_AS_STRING(argv[0]);
    char* buffer     = malloc(str->length + 1);
    for (i32 i = 0; i < str->length; i++) {
        buffer[i] = tolower(str->str[i]);
    }
    buffer[str->length] = '\0';
    return OBJ_VAL(xen_obj_str_take(buffer, str->length));
}

xen_value xen_str_trim(i32 argc, xen_value* argv) {
    if (argc < 1 || !OBJ_IS_STRING(argv[0]))
        return NULL_VAL;
    xen_obj_str* str = OBJ_AS_STRING(argv[0]);

    const char* start = str->str;
    const char* end   = str->str + str->length - 1;

    while (start <= end && isspace(*start))
        start++;
    while (end > start && isspace(*end))
        end--;

    i32 new_len = (i32)(end - start + 1);
    return OBJ_VAL(xen_obj_str_copy(start, new_len));
}

xen_value xen_str_contains(i32 argc, xen_value* argv) {
    if (argc < 2 || !OBJ_IS_STRING(argv[0]) || !OBJ_IS_STRING(argv[1]))
        return BOOL_VAL(XEN_FALSE);
    xen_obj_str* haystack = OBJ_AS_STRING(argv[0]);
    xen_obj_str* needle   = OBJ_AS_STRING(argv[1]);
    return BOOL_VAL(strstr(haystack->str, needle->str) != NULL);
}

xen_value xen_str_starts_with(i32 argc, xen_value* argv) {
    if (argc < 2 || !OBJ_IS_STRING(argv[0]) || !OBJ_IS_STRING(argv[1]))
        return BOOL_VAL(XEN_FALSE);
    xen_obj_str* str    = OBJ_AS_STRING(argv[0]);
    xen_obj_str* prefix = OBJ_AS_STRING(argv[1]);
    if (prefix->length > str->length)
        return BOOL_VAL(XEN_FALSE);
    return BOOL_VAL(memcmp(str->str, prefix->str, prefix->length) == 0);
}

xen_value xen_str_ends_with(i32 argc, xen_value* argv) {
    if (argc < 2 || !OBJ_IS_STRING(argv[0]) || !OBJ_IS_STRING(argv[1]))
        return BOOL_VAL(XEN_FALSE);
    xen_obj_str* str    = OBJ_AS_STRING(argv[0]);
    xen_obj_str* suffix = OBJ_AS_STRING(argv[1]);
    if (suffix->length > str->length)
        return BOOL_VAL(XEN_FALSE);
    const char* start = str->str + str->length - suffix->length;
    return BOOL_VAL(memcmp(start, suffix->str, suffix->length) == 0);
}

xen_value xen_str_substr(i32 argc, xen_value* argv) {
    if (argc < 2 || !OBJ_IS_STRING(argv[0]) || !VAL_IS_NUMBER(argv[1]))
        return NULL_VAL;

    xen_obj_str* str = OBJ_AS_STRING(argv[0]);
    i32 start        = (i32)VAL_AS_NUMBER(argv[1]);
    i32 len          = (argc >= 3 && VAL_IS_NUMBER(argv[2])) ? (i32)VAL_AS_NUMBER(argv[2]) : str->length - start;

    if (start < 0)
        start = 0;
    if (start >= str->length)
        return OBJ_VAL(xen_obj_str_copy("", 0));
    if (start + len > str->length)
        len = str->length - start;

    return OBJ_VAL(xen_obj_str_copy(str->str + start, len));
}

xen_value xen_str_find(i32 argc, xen_value* argv) {
    if (argc < 2 || !OBJ_IS_STRING(argv[0]) || !OBJ_IS_STRING(argv[1]))
        return NUMBER_VAL(-1);

    xen_obj_str* haystack = OBJ_AS_STRING(argv[0]);
    xen_obj_str* needle   = OBJ_AS_STRING(argv[1]);

    char* found = strstr(haystack->str, needle->str);
    if (found) {
        return NUMBER_VAL(found - haystack->str);
    }
    return NUMBER_VAL(-1);
}

xen_value xen_str_split(i32 argc, xen_value* argv) {
    if (argc < 2 || !OBJ_IS_STRING(argv[0]) || !OBJ_IS_STRING(argv[1]))
        return NULL_VAL;

    xen_obj_str* str   = OBJ_AS_STRING(argv[0]);
    xen_obj_str* delim = OBJ_AS_STRING(argv[1]);

    xen_obj_array* result = xen_obj_array_new();

    if (delim->length == 0) {
        for (i32 i = 0; i < str->length; i++) {
            xen_obj_array_push(result, OBJ_VAL(xen_obj_str_copy(&str->str[i], 1)));
        }
        return OBJ_VAL(result);
    }

    const char* start = str->str;
    const char* end   = str->str + str->length;
    const char* pos;

    while ((pos = strstr(start, delim->str)) != NULL) {
        i32 len = (i32)(pos - start);
        xen_obj_array_push(result, OBJ_VAL(xen_obj_str_copy(start, len)));
        start = pos + delim->length;
    }

    if (start < end) {
        xen_obj_array_push(result, OBJ_VAL(xen_obj_str_copy(start, (i32)(end - start))));
    }

    return OBJ_VAL(result);
}

xen_value xen_str_replace(i32 argc, xen_value* argv) {
    if (argc < 3 || !OBJ_IS_STRING(argv[0]) || !OBJ_IS_STRING(argv[1]) || !OBJ_IS_STRING(argv[2]))
        return argv[0];

    xen_obj_str* str     = OBJ_AS_STRING(argv[0]);
    xen_obj_str* find    = OBJ_AS_STRING(argv[1]);
    xen_obj_str* replace = OBJ_AS_STRING(argv[2]);

    if (find->length == 0)
        return argv[0];

    i32 count       = 0;
    const char* pos = str->str;
    while ((pos = strstr(pos, find->str)) != NULL) {
        count++;
        pos += find->length;
    }

    if (count == 0)
        return argv[0];

    i32 new_len  = str->length + count * (replace->length - find->length);
    char* buffer = malloc(new_len + 1);
    char* dest   = buffer;

    const char* src = str->str;
    while ((pos = strstr(src, find->str)) != NULL) {
        i32 prefix_len = (i32)(pos - src);
        memcpy(dest, src, prefix_len);
        dest += prefix_len;
        memcpy(dest, replace->str, replace->length);
        dest += replace->length;
        src = pos + find->length;
    }

    strcpy(dest, src);

    return OBJ_VAL(xen_obj_str_take(buffer, new_len));
}

xen_obj_namespace* xen_builtin_string() {
    xen_obj_namespace* str = xen_obj_namespace_new("string");
    xen_obj_namespace_set(str, "len", OBJ_VAL(xen_obj_native_func_new(xen_str_len, "len")));
    xen_obj_namespace_set(str, "upper", OBJ_VAL(xen_obj_native_func_new(xen_str_upper, "upper")));
    xen_obj_namespace_set(str, "lower", OBJ_VAL(xen_obj_native_func_new(xen_str_lower, "lower")));
    xen_obj_namespace_set(str, "substr", OBJ_VAL(xen_obj_native_func_new(xen_str_substr, "substr")));
    xen_obj_namespace_set(str, "find", OBJ_VAL(xen_obj_native_func_new(xen_str_find, "find")));
    xen_obj_namespace_set(str, "trim", OBJ_VAL(xen_obj_native_func_new(xen_str_trim, "trim")));
    xen_obj_namespace_set(str, "contains", OBJ_VAL(xen_obj_native_func_new(xen_str_contains, "contains")));
    xen_obj_namespace_set(str, "starts_with", OBJ_VAL(xen_obj_native_func_new(xen_str_starts_with, "starts_with")));
    xen_obj_namespace_set(str, "ends_with", OBJ_VAL(xen_obj_native_func_new(xen_str_ends_with, "ends_with")));
    xen_obj_namespace_set(str, "split", OBJ_VAL(xen_obj_native_func_new(xen_str_split, "split")));
    xen_obj_namespace_set(str, "replace", OBJ_VAL(xen_obj_native_func_new(xen_str_replace, "replace")));
    return str;
}