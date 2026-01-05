#include "xmem.h"
#include "xerr.h"
#include "xvalue.h"
#include "xobject.h"
#include "xvm.h"

void xen_vm_mem_init(xen_vm_mem* mem, size_t size_perm, size_t size_gen, size_t size_temp) {
    mem->permanent  = xen_alloc_create(size_perm);
    mem->generation = xen_alloc_create(size_gen);
    mem->temporary  = xen_alloc_create(size_temp);
}

void xen_vm_mem_destroy(xen_vm_mem* mem) {
    xen_alloc_destroy(mem->permanent);
    xen_alloc_destroy(mem->generation);
    xen_alloc_destroy(mem->temporary);
}

void* xen_mem_realloc(void* ptr, size_t old_size, size_t new_size) {
    if (new_size == 0) {
        free(ptr);
        return NULL;
    }

    void* result = realloc(ptr, new_size);
    if (result == NULL)
        xen_panic(XEN_ERR_ALLOCATION_FAILED, "failed to reallocate memory");

    return result;
}

void xen_mem_free_objects() {
    xen_obj* obj = g_vm.objects;
    while (obj != NULL) {
        xen_obj* next = obj->next;
        xen_mem_free_object(obj);
        obj = next;
    }
}

// TODO:  (URGENT) CLEANUP THE REST OF THE OBJ TYPES
static void xen_mem_free_object(xen_obj* obj) {
    switch (obj->type) {
        case OBJ_STRING: {
            const xen_obj_str* str = (xen_obj_str*)obj;
            XEN_FREE_ARRAY(char, str->str, str->length + 1);
            XEN_FREE(xen_obj_str, obj);
            break;
        }
        case OBJ_FUNCTION:
        case OBJ_NATIVE_FUNC:
        case OBJ_NAMESPACE:
        case OBJ_ARRAY:
        case OBJ_BOUND_METHOD:
        case OBJ_DICT:
        case OBJ_CLASS:
        case OBJ_INSTANCE:
            break;
    }
}

xen_ring_buffer* xen_ring_buffer_create(size_t capacity) {
    xen_ring_buffer* rb = (xen_ring_buffer*)malloc(sizeof(xen_ring_buffer));
    if (!rb)
        return NULL;

    rb->buffer = calloc(capacity, sizeof(char*));
    if (!rb->buffer) {
        free(rb);
        return NULL;
    }

    rb->head     = 0;
    rb->tail     = 0;
    rb->count    = 0;
    rb->capacity = capacity;

    return rb;
}

bool xen_ring_buffer_push(xen_ring_buffer* rb, const char* str) {
    if (!str)
        return XEN_FALSE;

    if (rb->count == rb->capacity && rb->buffer[rb->head] != NULL) {
        free(rb->buffer[rb->head]);
    }

    rb->buffer[rb->head] = malloc(strlen(str) + 1);
    if (!rb->buffer[rb->head])
        return XEN_FALSE;
    strcpy(rb->buffer[rb->head], str);

    if (rb->count == rb->capacity) {
        rb->tail = (rb->tail + 1) % rb->capacity;
    } else {
        rb->count++;
    }

    rb->head = (rb->head + 1) % rb->capacity;
    return XEN_FALSE;
}

bool xen_ring_buffer_pop(xen_ring_buffer* rb, char** str) {
    if (rb->count == 0)
        return XEN_FALSE;

    *str                 = rb->buffer[rb->tail];
    rb->buffer[rb->tail] = NULL;
    rb->tail             = (rb->tail + 1) % rb->capacity;
    rb->count--;

    return XEN_TRUE;
}

const char* xen_ring_buffer_peek(xen_ring_buffer* rb, size_t index) {
    if (index >= rb->count)
        return NULL;
    size_t actual_index = (rb->tail + index) % rb->capacity;
    return rb->buffer[actual_index];
}

bool xen_ring_buffer_is_full(xen_ring_buffer* rb) {
    return rb->count == rb->capacity;
}

void xen_ring_buffer_free(xen_ring_buffer* rb) {
    if (rb) {
        for (size_t i = 0; i < rb->capacity; i++) {
            if (rb->buffer[i] != NULL) {
                free(rb->buffer[i]);
            }
        }
        free(rb->buffer);
        free(rb);
    }
}

size_t xen_ring_buffer_count(xen_ring_buffer* rb) {
    if (!rb)
        return 0;
    return rb->count;
}

// Peek at a string from the end (0 = most recent, 1 = second most recent, etc.)
const char* xen_ring_buffer_peek_from_end(xen_ring_buffer* rb, size_t index) {
    if (!rb || index >= rb->count)
        return NULL;

    // Calculate the actual index by going backwards from head
    // head points to where the NEXT item will be written, so head-1 is most recent
    size_t actual_index;
    if (rb->head == 0) {
        // Wrap around
        actual_index = rb->capacity - 1 - index;
    } else {
        actual_index = (rb->head - 1 - index + rb->capacity) % rb->capacity;
    }

    return rb->buffer[actual_index];
}
