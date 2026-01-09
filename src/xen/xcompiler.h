#ifndef X_COMPILER_H
#define X_COMPILER_H

#include "xcommon.h"
#include "xchunk.h"
#include "xobject.h"
#include "xscanner.h"

xen_obj_func* xen_compile(const char* source);

#endif
