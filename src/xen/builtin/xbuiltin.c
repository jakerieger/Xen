#include "xbuiltin.h"
#include "../xcommon.h"
#include "../xerr.h"
#include "../xvalue.h"
#include "../xutils.h"
#include "../xtypeid.h"
#include "xbuiltin_common.h"

#include <ctype.h>
#include <time.h>

static xen_value xen_builtin_typeof(i32 argc, array(xen_value) argv) {
    REQUIRE_ARG("type", 0, TYPEID_UNDEFINED);
    xen_value val        = argv[0];
    i32 typeid           = xen_typeid_get(val);
    const char* type_str = xen_typeid_str(typeid);

    return OBJ_VAL(xen_obj_str_copy(type_str, strlen(type_str)));
}

static xen_value xen_builtin_typeid(i32 argc, array(xen_value) argv) {
    REQUIRE_ARG("type", 0, TYPEID_UNDEFINED);
    xen_value val = argv[0];
    i32 typeid    = xen_typeid_get(val);

    return NUMBER_VAL(typeid);
}

void xen_builtins_register() {
    srand((u32)time(NULL));
    xen_vm_register_namespace("math", OBJ_VAL(xen_builtin_math()));
    xen_vm_register_namespace("io", OBJ_VAL(xen_builtin_io()));
    xen_vm_register_namespace("string", OBJ_VAL(xen_builtin_string()));
    xen_vm_register_namespace("datetime", OBJ_VAL(xen_builtin_datetime()));
    xen_vm_register_namespace("array", OBJ_VAL(xen_builtin_array()));
    xen_vm_register_namespace("os", OBJ_VAL(xen_builtin_os()));
    xen_vm_register_namespace("dictionary", OBJ_VAL(xen_builtin_dict()));
    xen_vm_register_namespace("net", OBJ_VAL(xen_builtin_net()));

    // register globals
    define_native_fn("typeof", xen_builtin_typeof);
    define_native_fn("typeid", xen_builtin_typeid);

    // type constructors (uses capitalization to distinguish from namespaces)
    define_native_fn("Number", xen_builtin_number_ctor);
    define_native_fn("String", xen_builtin_string_ctor);
    define_native_fn("Bool", xen_builtin_bool_ctor);
    define_native_fn("Array", xen_builtin_array_ctor);
    define_native_fn("Dictionary", xen_builtin_dict_ctor);
    define_native_fn("UInt8Array", xen_builtin_u8array_ctor);
    define_native_fn("Error", xen_builtin_error_ctor);
}

void xen_vm_register_namespace(const char* name, xen_value ns) {
    xen_obj_str* name_str = xen_obj_str_copy(name, (i32)strlen(name));
    xen_table_set(&g_vm.namespace_registry, name_str, ns);
}
