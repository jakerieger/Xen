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
    i32 typeid = xen_typeid_get(val);

    switch (typeid) {
        case TYPEID_BOOL:
            type_str = "Bool";
            break;
        case TYPEID_NULL:
            type_str = "Null";
            break;
        case TYPEID_NUMBER:
            type_str = "Number";
            break;
        case TYPEID_STRING:
            type_str = "String";
            break;
        case TYPEID_FUNC:
            type_str = "Function";
            break;
        case TYPEID_NATIVE_FUNC:
            type_str = "Native_Function";
            break;
        case TYPEID_NAMESPACE:
            type_str = "Namespace";
            break;
        case TYPEID_ARRAY:
            type_str = "Array";
            break;
        case TYPEID_BOUND_METHOD:
            type_str = "Bound_Method";
            break;
        case TYPEID_DICT:
            type_str = "Dictionary";
            break;
        case TYPEID_CLASS:
            type_str = "Class";
            break;
        case TYPEID_INSTANCE:
            type_str = "Instance";
            break;
        case TYPEID_U8ARRAY:
            type_str = "U8Array";
            break;
        case TYPEID_UNDEFINED:
        default:
            type_str = "Undefined";
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