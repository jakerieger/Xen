#ifndef X_OBJ_ERROR_H
#define X_OBJ_ERROR_H

#include "xobj.h"
#include "xobj_string.h"

struct xen_obj_error {
    xen_obj obj;
    xen_obj_str* msg;
};

#define OBJ_IS_ERROR(v) xen_obj_is_type(v, OBJ_ERROR)
#define OBJ_AS_ERROR(v) ((xen_obj_error*)VAL_AS_OBJ(v))
#define OBJ_ERROR_GET_MSG_CSTR(v) ((OBJ_AS_ERROR(v))->msg->str)

xen_obj_error* xen_obj_error_new(const char* msg);
xen_value xen_obj_error_msg(i32 argc, xen_value* argv);

static xen_method_entry k_error_methods[] = {
  {"msg", xen_obj_error_msg, XEN_FALSE},
  {NULL, NULL, false},
};

#endif
