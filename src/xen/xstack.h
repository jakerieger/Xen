#ifndef X_STACK_H
#define X_STACK_H

#include "xalloc.h"
#include "xvalue.h"

typedef struct {
    u64 cap;
    u64 count;
    xen_value* stack;
    xen_value* top;
} xen_stack;

void xen_stack_init(xen_allocator* alloc, xen_stack* stack, size_t size);
i32 xen_stack_push(xen_stack* stack, xen_value value);
xen_value xen_stack_pop(xen_stack* stack);
void xen_stack_reset(xen_stack* stack);

#endif
