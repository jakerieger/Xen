#ifndef X_BUILTIN_H
#define X_BUILTIN_H

#include "../xcommon.h"
#include "../xvalue.h"
#include "../xvm.h"

#include "xbuiltin_common.h"
#include "xbuiltin_array.h"
#include "xbuiltin_datetime.h"
#include "xbuiltin_dict.h"
#include "xbuiltin_io.h"
#include "xbuiltin_math.h"
#include "xbuiltin_os.h"
#include "xbuiltin_string.h"
#include "xbuiltin_net.h"

void xen_builtins_register();
void xen_vm_register_namespace(const char* name, xen_value ns);

static const char* xen_builtin_namespaces[] = {
  "io",
  "math",
  "string",
  "datetime",
  "array",
  "dict",
  "os",
  "net",
  NULL,  // sentinel
};

#endif