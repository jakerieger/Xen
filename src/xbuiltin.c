#include "xbuiltin.h"
#include "xcommon.h"
#include "xerr.h"
#include "xobject.h"
#include "xvalue.h"
#include "xutils.h"
#include <ctype.h>
#include <time.h>

#define ENFORCE_NUM_ARGS(n)                                                                                            \
    if (argc != n) {                                                                                                   \
        xen_runtime_error("incorrect number of arguments provided: '%d' (requires %d)", argc, n);                      \
        return NULL_VAL;                                                                                               \
    }

static void define_native_fn(const char* name, xen_native_fn fn) {
    xen_obj_str* key = xen_obj_str_copy(name, strlen(name));
    xen_table_set(&g_vm.globals, key, OBJ_VAL(xen_obj_native_func_new(fn, name)));
}

static xen_value xen_builtin_typeof(i32 argc, xen_value* args) {
    ENFORCE_NUM_ARGS(1);

    xen_value val = args[0];
    const char* type_str;

    if (VAL_IS_BOOL(val)) {
        type_str = "bool";
    } else if (VAL_IS_NULL(val)) {
        type_str = "null";
    } else if (VAL_IS_NUMBER(val)) {
        type_str = "number";
    } else if (VAL_IS_OBJ(val)) {
        switch (OBJ_TYPE(val)) {
            case OBJ_STRING:
                type_str = "string";
                break;
            case OBJ_FUNCTION:
                type_str = "function";
                break;
            case OBJ_NATIVE_FUNC:
                type_str = "native_function";
                break;
            case OBJ_NAMESPACE:
                type_str = "namespace";
                break;
            default:
                type_str = "undefined";
                break;
        }
    } else {
        type_str = "undefined";
    }

    return OBJ_VAL(xen_obj_str_copy(type_str, strlen(type_str)));
}

/* type constructors */
static xen_value xen_builtin_number_ctor(i32 argc, xen_value* args) {
    if (argc != 1) {
        xen_runtime_error("number constructor requires an argument");
        return NULL_VAL;
    }
    xen_value val = args[0];
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

static xen_value xen_builtin_string_ctor(i32 argc, xen_value* args) {
    if (argc != 1) {
        xen_runtime_error("number constructor requires an argument");
        return NULL_VAL;
    }
    xen_value val = args[0];
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

static xen_value xen_builtin_bool_ctor(i32 argc, xen_value* args) {
    if (argc != 1) {
        xen_runtime_error("number constructor requires an argument");
        return NULL_VAL;
    }
    xen_value val = args[0];
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

/* signature: (number of elements, default value (or null if missing)) */
static xen_value xen_builtin_array_ctor(i32 argc, xen_value* args) {
    if (argc > 2) {
        xen_runtime_error("array constructor has invalid number of arguments");
        return NULL_VAL;
    }

    if (argc > 0 && args[0].type != VAL_NUMBER) {
        xen_runtime_error("element count must be a number");
        return NULL_VAL;
    }

    i32 element_count       = (argc > 0) ? VAL_AS_NUMBER(args[0]) : 0;
    bool has_default        = (argc == 2) ? XEN_TRUE : XEN_FALSE;
    xen_value default_value = (has_default) ? args[1] : NULL_VAL;

    /* create the array */
    xen_obj_array* arr = xen_obj_array_new_with_capacity(element_count);
    for (i32 i = 0; i < element_count; i++) {
        xen_obj_array_push(arr, default_value);
    }

    return OBJ_VAL(arr);
}

void xen_builtins_register() {
    srand((u32)time(NULL));
    xen_vm_register_namespace("math", OBJ_VAL(xen_builtin_math()));
    xen_vm_register_namespace("io", OBJ_VAL(xen_builtin_io()));
    xen_vm_register_namespace("string", OBJ_VAL(xen_builtin_string()));
    xen_vm_register_namespace("datetime", OBJ_VAL(xen_builtin_datetime()));
    xen_vm_register_namespace("array", OBJ_VAL(xen_builtin_array()));

    /* register globals */
    define_native_fn("typeof", xen_builtin_typeof);

    /* type constructors */
    define_native_fn("number", xen_builtin_number_ctor);
    define_native_fn("string", xen_builtin_string_ctor);
    define_native_fn("bool", xen_builtin_bool_ctor);
    define_native_fn("array", xen_builtin_array_ctor);
}

void xen_vm_register_namespace(const char* name, xen_value ns) {
    xen_obj_str* name_str = xen_obj_str_copy(name, (i32)strlen(name));
    xen_table_set(&g_vm.namespace_registry, name_str, ns);
}

// ============================================================================
// Math namespace
// ============================================================================

#include <math.h>

static xen_value math_sqrt(i32 argc, xen_value* args) {
    if (argc < 1 || !VAL_IS_NUMBER(args[0]))
        return NULL_VAL;
    return NUMBER_VAL(sqrt(VAL_AS_NUMBER(args[0])));
}

static xen_value math_sin(i32 argc, xen_value* args) {
    if (argc < 1 || !VAL_IS_NUMBER(args[0]))
        return NULL_VAL;
    return NUMBER_VAL(sin(VAL_AS_NUMBER(args[0])));
}

static xen_value math_cos(i32 argc, xen_value* args) {
    if (argc < 1 || !VAL_IS_NUMBER(args[0]))
        return NULL_VAL;
    return NUMBER_VAL(cos(VAL_AS_NUMBER(args[0])));
}

static xen_value math_tan(i32 argc, xen_value* args) {
    if (argc < 1 || !VAL_IS_NUMBER(args[0]))
        return NULL_VAL;
    return NUMBER_VAL(tan(VAL_AS_NUMBER(args[0])));
}

static xen_value math_pow(i32 argc, xen_value* args) {
    if (argc < 2 || !VAL_IS_NUMBER(args[0]) || !VAL_IS_NUMBER(args[1]))
        return NULL_VAL;
    return NUMBER_VAL(pow(VAL_AS_NUMBER(args[0]), VAL_AS_NUMBER(args[1])));
}

static xen_value math_log(i32 argc, xen_value* args) {
    if (argc < 1 || !VAL_IS_NUMBER(args[0]))
        return NULL_VAL;
    return NUMBER_VAL(log(VAL_AS_NUMBER(args[0])));
}

static xen_value math_log10(i32 argc, xen_value* args) {
    if (argc < 1 || !VAL_IS_NUMBER(args[0]))
        return NULL_VAL;
    return NUMBER_VAL(log10(VAL_AS_NUMBER(args[0])));
}

static xen_value math_exp(i32 argc, xen_value* args) {
    if (argc < 1 || !VAL_IS_NUMBER(args[0]))
        return NULL_VAL;
    return NUMBER_VAL(exp(VAL_AS_NUMBER(args[0])));
}

static xen_value math_min(i32 argc, xen_value* args) {
    if (argc < 1)
        return NULL_VAL;
    f64 min = VAL_AS_NUMBER(args[0]);
    for (i32 i = 1; i < argc; i++) {
        if (VAL_IS_NUMBER(args[i])) {
            f64 val = VAL_AS_NUMBER(args[i]);
            if (val < min)
                min = val;
        }
    }
    return NUMBER_VAL(min);
}

static xen_value math_max(i32 argc, xen_value* args) {
    if (argc < 1)
        return NULL_VAL;
    f64 max = VAL_AS_NUMBER(args[0]);
    for (i32 i = 1; i < argc; i++) {
        if (VAL_IS_NUMBER(args[i])) {
            f64 val = VAL_AS_NUMBER(args[i]);
            if (val > max)
                max = val;
        }
    }
    return NUMBER_VAL(max);
}

static xen_value math_random(i32 argc, xen_value* args) {
    XEN_UNUSED(argc);
    XEN_UNUSED(args);
    return NUMBER_VAL((f64)rand() / (f64)RAND_MAX);
}

xen_obj_namespace* xen_builtin_math() {
    xen_obj_namespace* math = xen_obj_namespace_new("math");
    xen_obj_namespace_set(math, "sqrt", OBJ_VAL(xen_obj_native_func_new(math_sqrt, "sqrt")));
    xen_obj_namespace_set(math, "abs", OBJ_VAL(xen_obj_native_func_new(xen_num_abs, "abs")));
    xen_obj_namespace_set(math, "floor", OBJ_VAL(xen_obj_native_func_new(xen_num_floor, "floor")));
    xen_obj_namespace_set(math, "ceil", OBJ_VAL(xen_obj_native_func_new(xen_num_ceil, "ceil")));
    xen_obj_namespace_set(math, "round", OBJ_VAL(xen_obj_native_func_new(xen_num_round, "round")));
    xen_obj_namespace_set(math, "sin", OBJ_VAL(xen_obj_native_func_new(math_sin, "sin")));
    xen_obj_namespace_set(math, "cos", OBJ_VAL(xen_obj_native_func_new(math_cos, "cos")));
    xen_obj_namespace_set(math, "tan", OBJ_VAL(xen_obj_native_func_new(math_tan, "tan")));
    xen_obj_namespace_set(math, "pow", OBJ_VAL(xen_obj_native_func_new(math_pow, "pow")));
    xen_obj_namespace_set(math, "log", OBJ_VAL(xen_obj_native_func_new(math_log, "log")));
    xen_obj_namespace_set(math, "log10", OBJ_VAL(xen_obj_native_func_new(math_log10, "log10")));
    xen_obj_namespace_set(math, "exp", OBJ_VAL(xen_obj_native_func_new(math_exp, "exp")));
    xen_obj_namespace_set(math, "min", OBJ_VAL(xen_obj_native_func_new(math_min, "min")));
    xen_obj_namespace_set(math, "max", OBJ_VAL(xen_obj_native_func_new(math_max, "max")));
    xen_obj_namespace_set(math, "random", OBJ_VAL(xen_obj_native_func_new(math_random, "random")));
    xen_obj_namespace_set(math, "PI", NUMBER_VAL(3.14159265358979323846));
    xen_obj_namespace_set(math, "E", NUMBER_VAL(2.71828182845904523536));
    xen_obj_namespace_set(math, "TAU", NUMBER_VAL(6.28318530717958647692));
    return math;
}

// ============================================================================
// IO namespace
// ============================================================================

static xen_value io_println(i32 argc, xen_value* args) {
    for (i32 i = 0; i < argc; i++) {
        xen_value_print(args[i]);
        if (i < argc - 1)
            printf(" ");
    }
    printf("\n");
    return NULL_VAL;
}

static xen_value io_print(i32 argc, xen_value* args) {
    for (i32 i = 0; i < argc; i++) {
        xen_value_print(args[i]);
    }
    return NULL_VAL;
}

static xen_value io_input(i32 argc, xen_value* args) {
    bool has_prefix = (argc > 0 && args[0].type == VAL_OBJECT && OBJ_IS_STRING(args[0]));
    if (has_prefix)
        printf("%s", OBJ_AS_CSTRING(args[0]));

    char buffer[1024];
    if (fgets(buffer, sizeof(buffer), stdin)) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
            len--;
        }
        return OBJ_VAL(xen_obj_str_copy(buffer, (i32)len));
    }
    return NULL_VAL;
}

static xen_value io_readtxt(i32 argc, xen_value* args) {
    if (argc != 1) {
        xen_runtime_error("io_readtxt() takes one argument (filename)");
        return NULL_VAL;
    }

    xen_value val = args[0];

    if (val.type == VAL_OBJECT && OBJ_IS_STRING(val)) {
        FILE* fp = fopen(OBJ_AS_CSTRING(val), "r");
        if (!fp) {
            xen_runtime_error("failed to open file: %s", OBJ_AS_CSTRING(val));
            return NULL_VAL;
        }

        fseek(fp, 0, SEEK_END);
        i32 fp_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        i32 input_size = fp_size + 1;
        char* buffer   = (char*)malloc(input_size);

        size_t read_bytes  = fread(buffer, 1, input_size, fp);
        buffer[read_bytes] = '\0';
        fclose(fp);

        return OBJ_VAL(xen_obj_str_take(buffer, input_size));
    } else {
        xen_runtime_error("filename argument must be of type string (got '%s')", xen_value_type_to_str(val));
        return NULL_VAL;
    }
}

static xen_value io_readlines(i32 argc, xen_value* args) {
    if (argc != 1) {
        xen_runtime_error("io_readlines() takes one argument (filename)");
        return NULL_VAL;
    }

    xen_value val = args[0];

    if (val.type == VAL_OBJECT && OBJ_IS_STRING(val)) {
        FILE* fp = fopen(OBJ_AS_CSTRING(val), "r");
        if (!fp) {
            xen_runtime_error("failed to open file: %s", OBJ_AS_CSTRING(val));
            return NULL_VAL;
        }

        xen_obj_array* arr = xen_obj_array_new();

        char* line;
        while ((line = xen_read_line(fp)) != NULL) {
            xen_obj_array_push(arr, OBJ_VAL(xen_obj_str_copy(line, strlen(line))));
            free(line);
        }

        fclose(fp);

        return OBJ_VAL(arr);
    } else {
        xen_runtime_error("filename argument must be of type string (got '%s')", xen_value_type_to_str(val));
        return NULL_VAL;
    }
}

xen_obj_namespace* xen_builtin_io() {
    xen_obj_namespace* io = xen_obj_namespace_new("io");
    xen_obj_namespace_set(io, "println", OBJ_VAL(xen_obj_native_func_new(io_println, "println")));
    xen_obj_namespace_set(io, "print", OBJ_VAL(xen_obj_native_func_new(io_print, "print")));
    xen_obj_namespace_set(io, "input", OBJ_VAL(xen_obj_native_func_new(io_input, "input")));
    xen_obj_namespace_set(io, "readtxt", OBJ_VAL(xen_obj_native_func_new(io_readtxt, "readtxt")));
    xen_obj_namespace_set(io, "readlines", OBJ_VAL(xen_obj_native_func_new(io_readlines, "readlines")));
    return io;
}

// ============================================================================
// String methods (exposed for type methods)
// ============================================================================

xen_value xen_str_len(i32 argc, xen_value* args) {
    if (argc < 1 || !OBJ_IS_STRING(args[0]))
        return NULL_VAL;
    return NUMBER_VAL(OBJ_AS_STRING(args[0])->length);
}

xen_value xen_str_upper(i32 argc, xen_value* args) {
    if (argc < 1 || !OBJ_IS_STRING(args[0]))
        return NULL_VAL;
    xen_obj_str* str = OBJ_AS_STRING(args[0]);
    char* buffer     = malloc(str->length + 1);
    for (i32 i = 0; i < str->length; i++) {
        buffer[i] = toupper(str->str[i]);
    }
    buffer[str->length] = '\0';
    return OBJ_VAL(xen_obj_str_take(buffer, str->length));
}

xen_value xen_str_lower(i32 argc, xen_value* args) {
    if (argc < 1 || !OBJ_IS_STRING(args[0]))
        return NULL_VAL;
    xen_obj_str* str = OBJ_AS_STRING(args[0]);
    char* buffer     = malloc(str->length + 1);
    for (i32 i = 0; i < str->length; i++) {
        buffer[i] = tolower(str->str[i]);
    }
    buffer[str->length] = '\0';
    return OBJ_VAL(xen_obj_str_take(buffer, str->length));
}

xen_value xen_str_trim(i32 argc, xen_value* args) {
    if (argc < 1 || !OBJ_IS_STRING(args[0]))
        return NULL_VAL;
    xen_obj_str* str = OBJ_AS_STRING(args[0]);

    const char* start = str->str;
    const char* end   = str->str + str->length - 1;

    while (start <= end && isspace(*start))
        start++;
    while (end > start && isspace(*end))
        end--;

    i32 new_len = (i32)(end - start + 1);
    return OBJ_VAL(xen_obj_str_copy(start, new_len));
}

xen_value xen_str_contains(i32 argc, xen_value* args) {
    if (argc < 2 || !OBJ_IS_STRING(args[0]) || !OBJ_IS_STRING(args[1]))
        return BOOL_VAL(XEN_FALSE);
    xen_obj_str* haystack = OBJ_AS_STRING(args[0]);
    xen_obj_str* needle   = OBJ_AS_STRING(args[1]);
    return BOOL_VAL(strstr(haystack->str, needle->str) != NULL);
}

xen_value xen_str_starts_with(i32 argc, xen_value* args) {
    if (argc < 2 || !OBJ_IS_STRING(args[0]) || !OBJ_IS_STRING(args[1]))
        return BOOL_VAL(XEN_FALSE);
    xen_obj_str* str    = OBJ_AS_STRING(args[0]);
    xen_obj_str* prefix = OBJ_AS_STRING(args[1]);
    if (prefix->length > str->length)
        return BOOL_VAL(XEN_FALSE);
    return BOOL_VAL(memcmp(str->str, prefix->str, prefix->length) == 0);
}

xen_value xen_str_ends_with(i32 argc, xen_value* args) {
    if (argc < 2 || !OBJ_IS_STRING(args[0]) || !OBJ_IS_STRING(args[1]))
        return BOOL_VAL(XEN_FALSE);
    xen_obj_str* str    = OBJ_AS_STRING(args[0]);
    xen_obj_str* suffix = OBJ_AS_STRING(args[1]);
    if (suffix->length > str->length)
        return BOOL_VAL(XEN_FALSE);
    const char* start = str->str + str->length - suffix->length;
    return BOOL_VAL(memcmp(start, suffix->str, suffix->length) == 0);
}

xen_value xen_str_substr(i32 argc, xen_value* args) {
    if (argc < 2 || !OBJ_IS_STRING(args[0]) || !VAL_IS_NUMBER(args[1]))
        return NULL_VAL;

    xen_obj_str* str = OBJ_AS_STRING(args[0]);
    i32 start        = (i32)VAL_AS_NUMBER(args[1]);
    i32 len          = (argc >= 3 && VAL_IS_NUMBER(args[2])) ? (i32)VAL_AS_NUMBER(args[2]) : str->length - start;

    if (start < 0)
        start = 0;
    if (start >= str->length)
        return OBJ_VAL(xen_obj_str_copy("", 0));
    if (start + len > str->length)
        len = str->length - start;

    return OBJ_VAL(xen_obj_str_copy(str->str + start, len));
}

xen_value xen_str_find(i32 argc, xen_value* args) {
    if (argc < 2 || !OBJ_IS_STRING(args[0]) || !OBJ_IS_STRING(args[1]))
        return NUMBER_VAL(-1);

    xen_obj_str* haystack = OBJ_AS_STRING(args[0]);
    xen_obj_str* needle   = OBJ_AS_STRING(args[1]);

    char* found = strstr(haystack->str, needle->str);
    if (found) {
        return NUMBER_VAL(found - haystack->str);
    }
    return NUMBER_VAL(-1);
}

xen_value xen_str_split(i32 argc, xen_value* args) {
    if (argc < 2 || !OBJ_IS_STRING(args[0]) || !OBJ_IS_STRING(args[1]))
        return NULL_VAL;

    xen_obj_str* str   = OBJ_AS_STRING(args[0]);
    xen_obj_str* delim = OBJ_AS_STRING(args[1]);

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

xen_value xen_str_replace(i32 argc, xen_value* args) {
    if (argc < 3 || !OBJ_IS_STRING(args[0]) || !OBJ_IS_STRING(args[1]) || !OBJ_IS_STRING(args[2]))
        return args[0];

    xen_obj_str* str     = OBJ_AS_STRING(args[0]);
    xen_obj_str* find    = OBJ_AS_STRING(args[1]);
    xen_obj_str* replace = OBJ_AS_STRING(args[2]);

    if (find->length == 0)
        return args[0];

    i32 count       = 0;
    const char* pos = str->str;
    while ((pos = strstr(pos, find->str)) != NULL) {
        count++;
        pos += find->length;
    }

    if (count == 0)
        return args[0];

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

// ============================================================================
// Time namespace
// ============================================================================

static xen_value time_now(i32 argc, xen_value* args) {
    (void)argc;
    (void)args;
    return NUMBER_VAL((f64)time(NULL));
}

static xen_value time_clock(i32 argc, xen_value* args) {
    (void)argc;
    (void)args;
    return NUMBER_VAL((f64)clock() / CLOCKS_PER_SEC);
}

xen_obj_namespace* xen_builtin_datetime() {
    xen_obj_namespace* t = xen_obj_namespace_new("datetime");
    xen_obj_namespace_set(t, "now", OBJ_VAL(xen_obj_native_func_new(time_now, "now")));
    xen_obj_namespace_set(t, "clock", OBJ_VAL(xen_obj_native_func_new(time_clock, "clock")));
    return t;
}

// ============================================================================
// Array methods (exposed for type methods)
// ============================================================================

xen_value xen_arr_len(i32 argc, xen_value* args) {
    if (argc < 1 || !OBJ_IS_ARRAY(args[0]))
        return NUMBER_VAL(-1);
    return NUMBER_VAL(OBJ_AS_ARRAY(args[0])->array.count);
}

xen_value xen_arr_push(i32 argc, xen_value* args) {
    if (argc < 2 || !OBJ_IS_ARRAY(args[0]))
        return NULL_VAL;
    xen_obj_array* arr = OBJ_AS_ARRAY(args[0]);
    for (i32 i = 1; i < argc; i++) {
        xen_obj_array_push(arr, args[i]);
    }
    return NUMBER_VAL(arr->array.count);
}

xen_value xen_arr_pop(i32 argc, xen_value* args) {
    if (argc < 1 || !OBJ_IS_ARRAY(args[0]))
        return NULL_VAL;
    return xen_obj_array_pop(OBJ_AS_ARRAY(args[0]));
}

xen_value xen_arr_first(i32 argc, xen_value* args) {
    if (argc < 1 || !OBJ_IS_ARRAY(args[0]))
        return NULL_VAL;
    xen_obj_array* arr = OBJ_AS_ARRAY(args[0]);
    if (arr->array.count == 0)
        return NULL_VAL;
    return arr->array.values[0];
}

xen_value xen_arr_last(i32 argc, xen_value* args) {
    if (argc < 1 || !OBJ_IS_ARRAY(args[0]))
        return NULL_VAL;
    xen_obj_array* arr = OBJ_AS_ARRAY(args[0]);
    if (arr->array.count == 0)
        return NULL_VAL;
    return arr->array.values[arr->array.count - 1];
}

xen_value xen_arr_clear(i32 argc, xen_value* args) {
    if (argc < 1 || !OBJ_IS_ARRAY(args[0]))
        return NULL_VAL;
    OBJ_AS_ARRAY(args[0])->array.count = 0;
    return NULL_VAL;
}

xen_value xen_arr_contains(i32 argc, xen_value* args) {
    if (argc < 2 || !OBJ_IS_ARRAY(args[0]))
        return BOOL_VAL(XEN_FALSE);
    xen_obj_array* arr = OBJ_AS_ARRAY(args[0]);
    xen_value needle   = args[1];
    for (i32 i = 0; i < arr->array.count; i++) {
        if (xen_value_equal(arr->array.values[i], needle)) {
            return BOOL_VAL(XEN_TRUE);
        }
    }
    return BOOL_VAL(XEN_FALSE);
}

xen_value xen_arr_index_of(i32 argc, xen_value* args) {
    if (argc < 2 || !OBJ_IS_ARRAY(args[0]))
        return NUMBER_VAL(-1);
    xen_obj_array* arr = OBJ_AS_ARRAY(args[0]);
    xen_value needle   = args[1];
    for (i32 i = 0; i < arr->array.count; i++) {
        if (xen_value_equal(arr->array.values[i], needle)) {
            return NUMBER_VAL(i);
        }
    }
    return NUMBER_VAL(-1);
}

xen_value xen_arr_reverse(i32 argc, xen_value* args) {
    if (argc < 1 || !OBJ_IS_ARRAY(args[0]))
        return NULL_VAL;
    xen_obj_array* arr = OBJ_AS_ARRAY(args[0]);
    for (i32 i = 0; i < arr->array.count / 2; i++) {
        i32 j                = arr->array.count - 1 - i;
        xen_value temp       = arr->array.values[i];
        arr->array.values[i] = arr->array.values[j];
        arr->array.values[j] = temp;
    }
    return args[0];
}

xen_value xen_arr_join(i32 argc, xen_value* args) {
    if (argc < 1 || !OBJ_IS_ARRAY(args[0]))
        return NULL_VAL;

    xen_obj_array* arr = OBJ_AS_ARRAY(args[0]);
    const char* sep    = ",";
    i32 sep_len        = 1;

    if (argc >= 2 && OBJ_IS_STRING(args[1])) {
        xen_obj_str* sep_str = OBJ_AS_STRING(args[1]);
        sep                  = sep_str->str;
        sep_len              = sep_str->length;
    }

    if (arr->array.count == 0) {
        return OBJ_VAL(xen_obj_str_copy("", 0));
    }

    i32 total_len = 0;
    for (i32 i = 0; i < arr->array.count; i++) {
        if (OBJ_IS_STRING(arr->array.values[i])) {
            total_len += OBJ_AS_STRING(arr->array.values[i])->length;
        } else if (VAL_IS_NUMBER(arr->array.values[i])) {
            total_len += 20;
        } else if (VAL_IS_BOOL(arr->array.values[i])) {
            total_len += 5;
        } else if (VAL_IS_NULL(arr->array.values[i])) {
            total_len += 4;
        }
        if (i < arr->array.count - 1) {
            total_len += sep_len;
        }
    }

    char* buffer = malloc(total_len + 1);
    char* ptr    = buffer;

    for (i32 i = 0; i < arr->array.count; i++) {
        if (OBJ_IS_STRING(arr->array.values[i])) {
            xen_obj_str* s = OBJ_AS_STRING(arr->array.values[i]);
            memcpy(ptr, s->str, s->length);
            ptr += s->length;
        } else if (VAL_IS_NUMBER(arr->array.values[i])) {
            ptr += sprintf(ptr, "%g", VAL_AS_NUMBER(arr->array.values[i]));
        } else if (VAL_IS_BOOL(arr->array.values[i])) {
            const char* b = VAL_AS_BOOL(arr->array.values[i]) ? "true" : "false";
            i32 len       = VAL_AS_BOOL(arr->array.values[i]) ? 4 : 5;
            memcpy(ptr, b, len);
            ptr += len;
        } else if (VAL_IS_NULL(arr->array.values[i])) {
            memcpy(ptr, "null", 4);
            ptr += 4;
        }

        if (i < arr->array.count - 1) {
            memcpy(ptr, sep, sep_len);
            ptr += sep_len;
        }
    }
    *ptr = '\0';

    return OBJ_VAL(xen_obj_str_take(buffer, (i32)(ptr - buffer)));
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

// ============================================================================
// Number methods (exposed for type methods)
// ============================================================================

xen_value xen_num_abs(i32 argc, xen_value* args) {
    if (argc < 1 || !VAL_IS_NUMBER(args[0]))
        return NULL_VAL;
    return NUMBER_VAL(fabs(VAL_AS_NUMBER(args[0])));
}

xen_value xen_num_floor(i32 argc, xen_value* args) {
    if (argc < 1 || !VAL_IS_NUMBER(args[0]))
        return NULL_VAL;
    return NUMBER_VAL(floor(VAL_AS_NUMBER(args[0])));
}

xen_value xen_num_ceil(i32 argc, xen_value* args) {
    if (argc < 1 || !VAL_IS_NUMBER(args[0]))
        return NULL_VAL;
    return NUMBER_VAL(ceil(VAL_AS_NUMBER(args[0])));
}

xen_value xen_num_round(i32 argc, xen_value* args) {
    if (argc < 1 || !VAL_IS_NUMBER(args[0]))
        return NULL_VAL;
    return NUMBER_VAL(round(VAL_AS_NUMBER(args[0])));
}

xen_value xen_num_to_string(i32 argc, xen_value* args) {
    if (argc < 1 || !VAL_IS_NUMBER(args[0]))
        return NULL_VAL;
    char buffer[32];
    i32 len = snprintf(buffer, sizeof(buffer), "%g", VAL_AS_NUMBER(args[0]));
    return OBJ_VAL(xen_obj_str_copy(buffer, len));
}
