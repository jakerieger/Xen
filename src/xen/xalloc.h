#ifndef X_ALLOC_H
#define X_ALLOC_H

#include "xcommon.h"

/*
 * Implements a simple arena alloctor for general-purpose
 * allocations.
 */

#define XEN_ALLOC_POS (sizeof(xen_allocator))

typedef struct {
    u64 cap;
    u64 pos;
} xen_allocator;

xen_allocator* xen_alloc_create(u64 capacity);
void xen_alloc_destroy(xen_allocator* alloc);
void* xen_alloc_push(xen_allocator* alloc, u64 size, bool non_zero);
void xen_alloc_pop(xen_allocator* alloc, u64 size);
void xen_alloc_pop_to(xen_allocator* alloc, u64 pos);
void xen_alloc_clear(xen_allocator* alloc);

#define XEN_ALLOC_ARRAY(alloc, type, count) (type*)xen_alloc_push(alloc, sizeof(type) * (count), XEN_FALSE)
#define XEN_ALLOC_TYPE(alloc, type) (type*)xen_alloc_push(alloc, sizeof(type), XEN_FALSE)

#endif
