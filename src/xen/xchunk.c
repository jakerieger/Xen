#include "xchunk.h"
#include "xalloc.h"
#include "xmem.h"
#include "xvalue.h"

void xen_chunk_init(xen_chunk* chunk) {
    chunk->count    = 0;
    chunk->capacity = 0;
    chunk->code     = NULL;
    chunk->lines    = NULL;
    xen_value_array_init(&chunk->constants);
}

void xen_chunk_write(xen_chunk* chunk, u8 byte, u64 line) {
    if (chunk->capacity < chunk->count + 1) {
        const i32 old_cap = chunk->capacity;
        chunk->capacity   = XEN_GROW_CAPACITY(old_cap);
        chunk->code       = XEN_GROW_ARRAY(u8, chunk->code, old_cap, chunk->capacity);
        chunk->lines      = XEN_GROW_ARRAY(u64, chunk->lines, old_cap, chunk->capacity);
    }

    chunk->code[chunk->count]  = byte;
    chunk->lines[chunk->count] = line;
    chunk->count++;
}

void xen_chunk_cleanup(xen_chunk* chunk) {
    XEN_FREE_ARRAY(u8, chunk->code, chunk->capacity);
    XEN_FREE_ARRAY(u64, chunk->lines, chunk->capacity);
    xen_value_array_free(&chunk->constants);
    xen_chunk_init(chunk);
}

i32 xen_chunk_add_constant(xen_chunk* chunk, xen_value value) {
    xen_value_array_write(&chunk->constants, value);
    return chunk->constants.count - 1;  // I don't remember why we're returning the index
}
