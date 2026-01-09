#include "xbuiltin_io.h"
#include "xbuiltin_common.h"

static xen_value io_println(i32 argc, xen_value* argv) {
    REQUIRE_ARG("msg", 0, TYPEID_UNDEFINED);
    for (i32 i = 0; i < argc; i++) {
        xen_value_print(argv[i]);
    }
    printf("\n");
    return NULL_VAL;
}

static xen_value io_print(i32 argc, xen_value* argv) {
    REQUIRE_ARG("msg", 0, TYPEID_UNDEFINED);
    for (i32 i = 0; i < argc; i++) {
        xen_value_print(argv[i]);
    }
    return NULL_VAL;
}

static xen_value io_input(i32 argc, xen_value* argv) {
    bool has_prefix = (argc > 0 && argv[0].type == VAL_OBJECT && OBJ_IS_STRING(argv[0]));
    if (has_prefix)
        printf("%s", OBJ_AS_CSTRING(argv[0]));

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

static xen_value io_clear(i32 argc, xen_value* argv) {
    XEN_UNUSED(argc);
    XEN_UNUSED(argv);
    printf("\033[2J\033[H");  // should be at least somewhat cross-platform
    return BOOL_VAL(XEN_TRUE);
}

static xen_value io_pause(i32 argc, xen_value* argv) {
    XEN_UNUSED(argc);
    XEN_UNUSED(argv);
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
    return NUMBER_VAL(c);
}

xen_obj_namespace* xen_builtin_io() {
    xen_obj_namespace* io = xen_obj_namespace_new("io");
    xen_obj_namespace_set(io, "println", OBJ_VAL(xen_obj_native_func_new(io_println, "println")));
    xen_obj_namespace_set(io, "print", OBJ_VAL(xen_obj_native_func_new(io_print, "print")));
    xen_obj_namespace_set(io, "input", OBJ_VAL(xen_obj_native_func_new(io_input, "input")));
    xen_obj_namespace_set(io, "clear", OBJ_VAL(xen_obj_native_func_new(io_clear, "clear")));
    xen_obj_namespace_set(io, "pause", OBJ_VAL(xen_obj_native_func_new(io_pause, "pause")));
    return io;
}