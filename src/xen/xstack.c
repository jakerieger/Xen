#include "xstack.h"

void xen_stack_init(xen_allocator* alloc, xen_stack* stack, size_t size) {
    stack->count = 0;
    stack->cap   = size;
    stack->stack = XEN_ALLOC_ARRAY(alloc, xen_value, size);
    stack->top   = stack->stack;
}

i32 xen_stack_push(xen_stack* stack, xen_value value) {
    if (stack->count + 1 > stack->cap) {
        xen_panic(XEN_ERR_OVER_CAPACITY, "failed to push to stack, maximum capacity");
    }

    *stack->top = value;
    stack->top++;

    return XEN_OK;
}

xen_value xen_stack_pop(xen_stack* stack) {
    stack->top--;
    return *stack->top;
}

void xen_stack_reset(xen_stack* stack) {
    stack->top = stack->stack;
}
