#ifndef X_VM_H
#define X_VM_H

#include "xalloc.h"
#include "xvm_config.h"
#include "xstack.h"
#include "xobject.h"
#include "xtable.h"
#include "xmem.h"
#include "xchunk.h"

#define FRAMES_MAX 64  // Maximum stack frames for a function
#define STACK_MAX (FRAMES_MAX * 256)

typedef struct {
    xen_obj_func* fn;
    u8* ip;
    xen_value* slots;  // Points into VM's value stack
} xen_call_frame;

typedef struct {
    xen_vm_mem mem;

    xen_call_frame frames[FRAMES_MAX];
    i32 frame_count;

    xen_value stack[STACK_MAX];
    xen_value* stack_top;

    xen_table strings;
    xen_table globals;
    xen_table const_globals;
    xen_table namespace_registry;
    xen_obj* objects;
} xen_vm;

typedef enum {
    EXEC_OK,
    EXEC_COMPILE_ERROR,
    EXEC_RUNTIME_ERROR,
} xen_exec_result;

extern xen_vm g_vm;

void xen_vm_init(xen_vm_config config);
void xen_vm_shutdown();
xen_exec_result xen_vm_exec(const char* source, bool emit_bytecode, const char* bytecode_filename);

#define MAX_OBJECT_COUNT 9999
#define MAX_STRINGS_COUNT 9999

#endif
