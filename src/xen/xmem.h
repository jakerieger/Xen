#ifndef X_MEM_H
#define X_MEM_H

#include "xalloc.h"

typedef struct {
    xen_allocator* permanent;   // GC roots, global state
    xen_allocator* generation;  // Current execution generation
    xen_allocator* temporary;   // Expression evaluation, stack frames
} xen_vm_mem;

void xen_vm_mem_init(xen_vm_mem* mem, size_t size_perm, size_t size_gen, size_t size_temp);
void xen_vm_mem_destroy(xen_vm_mem* mem);

typedef struct xen_obj xen_obj;

void* xen_mem_realloc(void* ptr, size_t old_size, size_t new_size);
void xen_mem_free_objects();
static void xen_mem_free_object(xen_obj* obj);

#define XEN_ALLOCATE(type, count) (type*)xen_mem_realloc(NULL, 0, sizeof(type) * (count))
#define XEN_GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)
#define XEN_GROW_ARRAY(type, pointer, old_count, new_count)                                                            \
    (type*)xen_mem_realloc(pointer, sizeof(type) * (old_count), sizeof(type) * (new_count))
#define XEN_FREE_ARRAY(type, pointer, old_count) xen_mem_realloc(pointer, sizeof(type) * (old_count), 0)
#define XEN_FREE(type, pointer) xen_mem_realloc(pointer, sizeof(type), 0)

typedef struct {
    char** buffer;
    size_t head;
    size_t tail;
    size_t capacity;
    size_t count;
} xen_ring_buffer;

xen_ring_buffer* xen_ring_buffer_create(size_t capacity);
bool xen_ring_buffer_push(xen_ring_buffer* rb, const char* str);
// Get the oldest string (returns false if empty)
// Note: Caller must free the returned string
bool xen_ring_buffer_pop(xen_ring_buffer* rb, char** str);
const char* xen_ring_buffer_peek(xen_ring_buffer* rb, size_t index);
bool xen_ring_buffer_is_full(xen_ring_buffer* rb);
void xen_ring_buffer_free(xen_ring_buffer* rb);
size_t xen_ring_buffer_count(xen_ring_buffer* rb);
const char* xen_ring_buffer_peek_from_end(xen_ring_buffer* rb, size_t index);

#endif
