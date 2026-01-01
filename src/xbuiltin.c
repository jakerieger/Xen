#include "xbuiltin.h"
#include "xcommon.h"
#include <ctype.h>
#include <time.h>

static void define_native_fn(const char* name, xen_native_fn fn) {
    xen_obj_str* str = xen_obj_str_copy(name, strlen(name));
    xen_table_set(&g_vm.globals, OBJ_AS_STRING(g_vm.stack[0]), g_vm.stack[1]);
}

xen_value xen_builtin_typeof(i32 arg_count, xen_value* args) {
    if (arg_count != 1) {
        return NULL_VAL;
    }

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
            default:
                type_str = "undefined";
                break;
        }
    } else {
        type_str = "undefined";
    }

    return OBJ_VAL(xen_obj_str_copy(type_str, strlen(type_str)));
}

void xen_builtins_register() {
    srand((u32)time(NULL));
    xen_vm_register_namespace("math", OBJ_VAL(xen_builtin_math()));
    xen_vm_register_namespace("io", OBJ_VAL(xen_builtin_io()));
    xen_vm_register_namespace("string", OBJ_VAL(xen_builtin_string()));
    xen_vm_register_namespace("datetime", OBJ_VAL(xen_builtin_datetime()));

    /* register globals */
    define_native_fn("typeof", xen_builtin_typeof);
}

void xen_vm_register_namespace(const char* name, xen_value ns) {
    xen_obj_str* name_str = xen_obj_str_copy(name, (i32)strlen(name));
    xen_table_set(&g_vm.namespace_registry, name_str, ns);
}

// ========================
// Math namespace
// ========================

#include <math.h>

static xen_value math_sqrt(i32 argc, xen_value* args) {
    if (argc != 1 || !VAL_IS_NUMBER(args[0])) {
        /* In production, set an error flag */
        return NULL_VAL;
    }
    return NUMBER_VAL(sqrt(VAL_AS_NUMBER(args[0])));
}

static xen_value math_abs(i32 argc, xen_value* args) {
    if (argc != 1 || !VAL_IS_NUMBER(args[0]))
        return NULL_VAL;
    return NUMBER_VAL(fabs(VAL_AS_NUMBER(args[0])));
}

static xen_value math_floor(i32 argc, xen_value* args) {
    if (argc != 1 || !VAL_AS_NUMBER(args[0]))
        return NULL_VAL;
    return NUMBER_VAL(floor(VAL_AS_NUMBER(args[0])));
}

static xen_value math_ceil(i32 argc, xen_value* args) {
    if (argc != 1 || !VAL_AS_NUMBER(args[0]))
        return NULL_VAL;
    return NUMBER_VAL(ceil(VAL_AS_NUMBER(args[0])));
}

static xen_value math_round(i32 argc, xen_value* args) {
    if (argc != 1 || !VAL_IS_NUMBER(args[0]))
        return NULL_VAL;
    return NUMBER_VAL(round(VAL_AS_NUMBER(args[0])));
}

static xen_value math_sin(i32 argc, xen_value* args) {
    if (argc != 1 || !VAL_IS_NUMBER(args[0]))
        return NULL_VAL;
    return NUMBER_VAL(sin(VAL_AS_NUMBER(args[0])));
}

static xen_value math_cos(i32 argc, xen_value* args) {
    if (argc != 1 || !VAL_IS_NUMBER(args[0]))
        return NULL_VAL;
    return NUMBER_VAL(cos(VAL_AS_NUMBER(args[0])));
}

static xen_value math_tan(i32 argc, xen_value* args) {
    if (argc != 1 || !VAL_IS_NUMBER(args[0]))
        return NULL_VAL;
    return NUMBER_VAL(tan(VAL_AS_NUMBER(args[0])));
}

static xen_value math_pow(i32 argc, xen_value* args) {
    if (argc != 2 || !VAL_IS_NUMBER(args[0]) || !VAL_IS_NUMBER(args[1]))
        return NULL_VAL;
    return NUMBER_VAL(pow(VAL_AS_NUMBER(args[0]), VAL_AS_NUMBER(args[1])));
}

static xen_value math_log(i32 argc, xen_value* args) {
    if (argc != 1 || !VAL_IS_NUMBER(args[0]))
        return NULL_VAL;
    return NUMBER_VAL(log(VAL_AS_NUMBER(args[0])));
}

static xen_value math_log10(i32 argc, xen_value* args) {
    if (argc != 1 || !VAL_IS_NUMBER(args[0]))
        return NULL_VAL;
    return NUMBER_VAL(log10(VAL_AS_NUMBER(args[0])));
}

static xen_value math_exp(i32 argc, xen_value* args) {
    if (argc != 1 || !VAL_IS_NUMBER(args[0]))
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
    xen_obj_namespace_set(math, "abs", OBJ_VAL(xen_obj_native_func_new(math_abs, "abs")));
    xen_obj_namespace_set(math, "floor", OBJ_VAL(xen_obj_native_func_new(math_floor, "floor")));
    xen_obj_namespace_set(math, "ceil", OBJ_VAL(xen_obj_native_func_new(math_ceil, "ceil")));
    xen_obj_namespace_set(math, "round", OBJ_VAL(xen_obj_native_func_new(math_round, "round")));
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

// ========================
// IO namespace
// ========================

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

static xen_value io_readline(i32 argc, xen_value* args) {
    (void)argc;
    (void)args;
    char buffer[1024];
    if (fgets(buffer, sizeof(buffer), stdin)) {
        /* remove trailing newline */
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
            len--;
        }
        return OBJ_VAL(xen_obj_str_copy(buffer, (i32)len));
    }
    return NULL_VAL;
}

xen_obj_namespace* xen_builtin_io() {
    xen_obj_namespace* io = xen_obj_namespace_new("io");
    xen_obj_namespace_set(io, "println", OBJ_VAL(xen_obj_native_func_new(io_println, "println")));
    xen_obj_namespace_set(io, "print", OBJ_VAL(xen_obj_native_func_new(io_print, "print")));
    xen_obj_namespace_set(io, "readline", OBJ_VAL(xen_obj_native_func_new(io_readline, "readline")));
    return io;
}

// ========================
// String namespace
// ========================

static xen_value string_len(i32 argc, xen_value* args) {
    if (argc != 1 || !OBJ_IS_STRING(args[0]))
        return NULL_VAL;
    return NUMBER_VAL(OBJ_AS_STRING(args[0])->length);
}

static xen_value string_upper(i32 argc, xen_value* args) {
    if (argc != 1 || !OBJ_IS_STRING(args[0]))
        return NULL_VAL;
    xen_obj_str* str = OBJ_AS_STRING(args[0]);
    char* buffer     = malloc(str->length + 1);
    for (i32 i = 0; i < str->length; i++) {
        buffer[i] = toupper(str->str[i]);
    }
    buffer[str->length] = '\0';
    xen_obj_str* result = xen_obj_str_take(buffer, str->length);
    return OBJ_VAL(result);
}

static xen_value string_lower(i32 argc, xen_value* args) {
    if (argc != 1 || !OBJ_IS_STRING(args[0]))
        return NULL_VAL;
    xen_obj_str* str = OBJ_AS_STRING(args[0]);
    char* buffer     = malloc(str->length + 1);
    for (i32 i = 0; i < str->length; i++) {
        buffer[i] = tolower(str->str[i]);
    }
    buffer[str->length] = '\0';
    xen_obj_str* result = xen_obj_str_take(buffer, str->length);
    return OBJ_VAL(result);
}

static xen_value string_substr(i32 argc, xen_value* args) {
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

static xen_value string_find(i32 argc, xen_value* args) {
    if (argc != 2 || !OBJ_IS_STRING(args[0]) || !OBJ_IS_STRING(args[1]))
        return NUMBER_VAL(-1);

    xen_obj_str* haystack = OBJ_AS_STRING(args[0]);
    xen_obj_str* needle   = OBJ_AS_STRING(args[1]);

    char* found = strstr(haystack->str, needle->str);
    if (found) {
        return NUMBER_VAL(found - haystack->str);
    }
    return NUMBER_VAL(-1);
}

static xen_value string_split(i32 argc, xen_value* args) {
    // This would return an array - you'd need arrays first
    // For now, return null as placeholder
    (void)argc;
    (void)args;
    return NULL_VAL;
}

static xen_value string_trim(i32 argc, xen_value* args) {
    if (argc != 1 || !OBJ_IS_STRING(args[0]))
        return NULL_VAL;

    xen_obj_str* str  = OBJ_AS_STRING(args[0]);
    const char* start = str->str;
    const char* end   = str->str + str->length - 1;

    while (start <= end && isspace(*start))
        start++;
    while (end > start && isspace(*end))
        end--;

    i32 new_len = (i32)(end - start + 1);
    return OBJ_VAL(xen_obj_str_copy(start, new_len));
}

static xen_value string_contains(i32 argc, xen_value* args) {
    if (argc != 2 || !OBJ_IS_STRING(args[0]) || !OBJ_IS_STRING(args[1]))
        return BOOL_VAL(XEN_FALSE);

    xen_obj_str* haystack = OBJ_AS_STRING(args[0]);
    xen_obj_str* needle   = OBJ_AS_STRING(args[1]);

    return BOOL_VAL(strstr(haystack->str, needle->str) != NULL);
}

static xen_value string_starts_with(i32 argc, xen_value* args) {
    if (argc != 2 || !OBJ_IS_STRING(args[0]) || !OBJ_IS_STRING(args[1]))
        return BOOL_VAL(XEN_FALSE);

    xen_obj_str* str    = OBJ_AS_STRING(args[0]);
    xen_obj_str* prefix = OBJ_AS_STRING(args[1]);

    if (prefix->length > str->length)
        return BOOL_VAL(XEN_FALSE);
    return BOOL_VAL(memcmp(str->str, prefix->str, prefix->length) == 0);
}

static xen_value string_ends_with(i32 argc, xen_value* args) {
    if (argc != 2 || !OBJ_IS_STRING(args[0]) || !OBJ_IS_STRING(args[1]))
        return BOOL_VAL(XEN_FALSE);

    xen_obj_str* str    = OBJ_AS_STRING(args[0]);
    xen_obj_str* suffix = OBJ_AS_STRING(args[1]);

    if (suffix->length > str->length)
        return BOOL_VAL(XEN_FALSE);
    const char* start = str->str + str->length - suffix->length;
    return BOOL_VAL(memcmp(start, suffix->str, suffix->length) == 0);
}

xen_obj_namespace* xen_builtin_string() {
    xen_obj_namespace* str = xen_obj_namespace_new("string");
    xen_obj_namespace_set(str, "len", OBJ_VAL(xen_obj_native_func_new(string_len, "len")));
    xen_obj_namespace_set(str, "upper", OBJ_VAL(xen_obj_native_func_new(string_upper, "upper")));
    xen_obj_namespace_set(str, "lower", OBJ_VAL(xen_obj_native_func_new(string_lower, "lower")));
    xen_obj_namespace_set(str, "substr", OBJ_VAL(xen_obj_native_func_new(string_substr, "substr")));
    xen_obj_namespace_set(str, "find", OBJ_VAL(xen_obj_native_func_new(string_find, "find")));
    xen_obj_namespace_set(str, "trim", OBJ_VAL(xen_obj_native_func_new(string_trim, "trim")));
    xen_obj_namespace_set(str, "contains", OBJ_VAL(xen_obj_native_func_new(string_contains, "contains")));
    xen_obj_namespace_set(str, "starts_with", OBJ_VAL(xen_obj_native_func_new(string_starts_with, "starts_with")));
    xen_obj_namespace_set(str, "ends_with", OBJ_VAL(xen_obj_native_func_new(string_ends_with, "ends_with")));
    return str;
}

// ========================
// Time namespace
// ========================

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
