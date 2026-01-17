#include "xobj_error.h"
#include "xobj_string.h"

xen_obj_error* xen_obj_error_new(const char* msg) {
    xen_obj_error* err = ALLOCATE_OBJ(xen_obj_error, OBJ_ERROR);
    err->msg           = xen_obj_str_copy(msg, strlen(msg));
    return err;
}

xen_value xen_obj_error_msg(i32 argc, xen_value* argv) {
    if (argc < 1 || !OBJ_IS_ERROR(argv[0]))
        return EMPTY_STRING_VAL;
    return OBJ_VAL(OBJ_AS_ERROR(argv[0])->msg);
}
