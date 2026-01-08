#include "xbuiltin.h"
#include "../xcommon.h"
#include "../xerr.h"
#include "../xobject.h"
#include "../xvalue.h"
#include "../xutils.h"
#include "../xtypeid.h"

#include <ctype.h>
#include <time.h>

static xen_value xen_builtin_typeof(i32 argc, xen_value* argv) {
    REQUIRE_ARG("type", 0, TYPEID_UNDEFINED);
    xen_value val = argv[0];
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
            case OBJ_ARRAY:
                type_str = "array";
                break;
            case OBJ_DICT:
                type_str = "dictionary";
                break;
            case OBJ_CLASS:
                type_str = "class";
                break;
            case OBJ_INSTANCE:
                type_str = "instance";
                break;
            case OBJ_BOUND_METHOD:
                type_str = "bound_method";
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
    xen_vm_register_namespace("array", OBJ_VAL(xen_builtin_array()));
    xen_vm_register_namespace("os", OBJ_VAL(xen_builtin_os()));
    xen_vm_register_namespace("dict", OBJ_VAL(xen_builtin_dict()));
    xen_vm_register_namespace("net", OBJ_VAL(xen_builtin_net()));

    // register globals
    define_native_fn("typeof", xen_builtin_typeof);

    // type constructors
    define_native_fn("number", xen_builtin_number_ctor);
    define_native_fn("string", xen_builtin_string_ctor);
    define_native_fn("bool", xen_builtin_bool_ctor);
    define_native_fn("array", xen_builtin_array_ctor);
    // TODO: Dictionary constructor
}

void xen_vm_register_namespace(const char* name, xen_value ns) {
    xen_obj_str* name_str = xen_obj_str_copy(name, (i32)strlen(name));
    xen_table_set(&g_vm.namespace_registry, name_str, ns);
}