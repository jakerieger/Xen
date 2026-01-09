#include "xbuiltin_datetime.h"
#include "xbuiltin_common.h"
#include <time.h>

static xen_value time_now(i32 argc, xen_value* argv) {
    XEN_UNUSED(argc);
    XEN_UNUSED(argv);
    return NUMBER_VAL((f64)time(NULL));
}

static xen_value time_clock(i32 argc, xen_value* argv) {
    XEN_UNUSED(argc);
    XEN_UNUSED(argv);
    return NUMBER_VAL((f64)clock() / CLOCKS_PER_SEC);
}

xen_obj_namespace* xen_builtin_datetime() {
    xen_obj_namespace* t = xen_obj_namespace_new("datetime");
    xen_obj_namespace_set(t, "now", OBJ_VAL(xen_obj_native_func_new(time_now, "now")));
    xen_obj_namespace_set(t, "clock", OBJ_VAL(xen_obj_native_func_new(time_clock, "clock")));
    return t;
}
