#include "xalloc.h"

xen_allocator* xen_alloc_create(u64 capacity) {
    xen_allocator* out = (xen_allocator*)malloc(capacity);
    if (!out) {
        xen_panic(XEN_ERR_ALLOCATION_FAILED, "failed to allocate memory for xen_allocator");
    }

    out->pos = XEN_ALLOC_POS;
    out->cap = capacity;

    return out;
}

void xen_alloc_destroy(xen_allocator* alloc) {
    free(alloc);
}

void* xen_alloc_push(xen_allocator* alloc, u64 size, bool non_zero) {
    u64 pos_aligned = XEN_ALIGN_UP(alloc->pos, XEN_PAGESIZE);
    u64 new_pos     = pos_aligned + size;

    if (new_pos > alloc->cap) {
        xen_panic(XEN_ERR_ALLOCATION_FAILED,
                 "attempted to allocate unavailable memory (capacity: %lu bytes, attempted: %lu bytes)",
                 alloc->cap,
                 new_pos);
    }

    alloc->pos = new_pos;
    u8* out    = (u8*)alloc + pos_aligned;

    if (!non_zero) {
        memset(out, 0, size);
    }

    return out;
}

void xen_alloc_pop(xen_allocator* alloc, u64 size) {
    size = XEN_MIN(size, alloc->pos - XEN_ALLOC_POS);
    alloc->pos -= size;
}

void xen_alloc_pop_to(xen_allocator* alloc, u64 pos) {
    u64 size = pos < alloc->pos ? alloc->pos - 1 : 0;
    xen_alloc_pop(alloc, size);
}

void xen_alloc_clear(xen_allocator* alloc) {
    xen_alloc_pop_to(alloc, XEN_ALLOC_POS);
}
