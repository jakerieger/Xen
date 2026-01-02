#ifndef X_CHUNK_H
#define X_CHUNK_H

#include "xcommon.h"
#include "xvalue.h"

typedef enum {
    OP_CONSTANT,
    OP_NULL,
    OP_TRUE,
    OP_FALSE,
    OP_NOT,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_NEGATE,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_MOD,
    OP_RETURN,
    OP_POP,
    OP_PRINT,
    OP_DEFINE_GLOBAL,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
    OP_CALL,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_LOOP,
    OP_INCLUDE,
    OP_GET_PROPERTY,
    OP_ARRAY_NEW,
    OP_ARRAY_GET,
    OP_ARRAY_SET,
    OP_ARRAY_LEN,
} xen_opcode;

typedef struct {
    u64 count;
    u64 capacity;
    u8* code;
    u64* lines;
    xen_value_array constants;
} xen_chunk;

void xen_chunk_init(xen_chunk* chunk);
void xen_chunk_write(xen_chunk* chunk, u8 byte, u64 line);
void xen_chunk_cleanup(xen_chunk* chunk);
i32 xen_chunk_add_constant(xen_chunk* chunk, xen_value value);

#endif
