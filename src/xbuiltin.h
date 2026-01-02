#ifndef X_BUILTIN_H
#define X_BUILTIN_H

#include "xcommon.h"
#include "xvm.h"

void xen_builtins_register();
void xen_vm_register_namespace(const char* name, xen_value ns);

xen_obj_namespace* xen_builtin_math();
xen_obj_namespace* xen_builtin_io();
xen_obj_namespace* xen_builtin_string();
xen_obj_namespace* xen_builtin_datetime();

#endif
