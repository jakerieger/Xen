#include "xbuiltin_math.h"
#include "xbuiltin_common.h"
#include <math.h>

static xen_value math_sqrt(i32 argc, xen_value* argv) {
    REQUIRE_ARG("value", 0, TYPEID_NUMBER);
    return NUMBER_VAL(sqrt(VAL_AS_NUMBER(argv[0])));
}

static xen_value math_sin(i32 argc, xen_value* argv) {
    REQUIRE_ARG("value", 0, TYPEID_NUMBER);
    return NUMBER_VAL(sin(VAL_AS_NUMBER(argv[0])));
}

static xen_value math_cos(i32 argc, xen_value* argv) {
    REQUIRE_ARG("value", 0, TYPEID_NUMBER);
    return NUMBER_VAL(cos(VAL_AS_NUMBER(argv[0])));
}

static xen_value math_tan(i32 argc, xen_value* argv) {
    REQUIRE_ARG("value", 0, TYPEID_NUMBER);
    return NUMBER_VAL(tan(VAL_AS_NUMBER(argv[0])));
}

static xen_value math_pow(i32 argc, xen_value* argv) {
    REQUIRE_ARG("base", 0, TYPEID_NUMBER);
    REQUIRE_ARG("exponent", 1, TYPEID_NUMBER);
    return NUMBER_VAL(pow(VAL_AS_NUMBER(argv[0]), VAL_AS_NUMBER(argv[1])));
}

static xen_value math_log(i32 argc, xen_value* argv) {
    REQUIRE_ARG("value", 0, TYPEID_NUMBER);
    return NUMBER_VAL(log(VAL_AS_NUMBER(argv[0])));
}

static xen_value math_log10(i32 argc, xen_value* argv) {
    REQUIRE_ARG("value", 0, TYPEID_NUMBER);
    return NUMBER_VAL(log10(VAL_AS_NUMBER(argv[0])));
}

static xen_value math_exp(i32 argc, xen_value* argv) {
    REQUIRE_ARG("value", 0, TYPEID_NUMBER);
    return NUMBER_VAL(exp(VAL_AS_NUMBER(argv[0])));
}

static xen_value math_min(i32 argc, xen_value* argv) {
    REQUIRE_ARG("value", 0, TYPEID_NUMBER);
    f64 min = VAL_AS_NUMBER(argv[0]);
    for (i32 i = 1; i < argc; i++) {
        if (VAL_IS_NUMBER(argv[i])) {
            f64 val = VAL_AS_NUMBER(argv[i]);
            if (val < min)
                min = val;
        }
    }
    return NUMBER_VAL(min);
}

static xen_value math_max(i32 argc, xen_value* argv) {
    REQUIRE_ARG("value", 0, TYPEID_NUMBER);
    f64 max = VAL_AS_NUMBER(argv[0]);
    for (i32 i = 1; i < argc; i++) {
        if (VAL_IS_NUMBER(argv[i])) {
            f64 val = VAL_AS_NUMBER(argv[i]);
            if (val > max)
                max = val;
        }
    }
    return NUMBER_VAL(max);
}

static xen_value math_random(i32 argc, xen_value* argv) {
    XEN_UNUSED(argc);
    XEN_UNUSED(argv);
    return NUMBER_VAL((f64)rand() / (f64)RAND_MAX);
}

static xen_value xen_num_abs(i32 argc, xen_value* argv) {
    if (argc < 1 || !VAL_IS_NUMBER(argv[0]))
        return NULL_VAL;
    return NUMBER_VAL(fabs(VAL_AS_NUMBER(argv[0])));
}

static xen_value xen_num_floor(i32 argc, xen_value* argv) {
    if (argc < 1 || !VAL_IS_NUMBER(argv[0]))
        return NULL_VAL;
    return NUMBER_VAL(floor(VAL_AS_NUMBER(argv[0])));
}

static xen_value xen_num_ceil(i32 argc, xen_value* argv) {
    if (argc < 1 || !VAL_IS_NUMBER(argv[0]))
        return NULL_VAL;
    return NUMBER_VAL(ceil(VAL_AS_NUMBER(argv[0])));
}

static xen_value xen_num_round(i32 argc, xen_value* argv) {
    if (argc < 1 || !VAL_IS_NUMBER(argv[0]))
        return NULL_VAL;
    return NUMBER_VAL(round(VAL_AS_NUMBER(argv[0])));
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