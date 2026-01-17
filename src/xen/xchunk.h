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
    OP_INVOKE,
    OP_INDEX_GET,
    OP_INDEX_SET,
    OP_ARRAY_NEW,
    OP_ARRAY_LEN,
    OP_DICT_NEW,
    OP_DICT_ADD,
    OP_CLASS,         // Create class object
    OP_SET_PROPERTY,  // Set property on instance
    OP_METHOD,        // Add method to class
    OP_PROPERTY,
    OP_INITIALIZER,
    OP_CALL_INIT,
    OP_IS_TYPE,  // Check if value is of a specific type
    OP_CAST,
} xen_opcode;

typedef struct {
    u64 count;
    u64 capacity;
    u8* code;
    array(u64) lines;
    xen_value_array constants;
} xen_chunk;

void xen_chunk_init(xen_chunk* chunk);
void xen_chunk_write(xen_chunk* chunk, u8 byte, u64 line);
void xen_chunk_cleanup(xen_chunk* chunk);
i32 xen_chunk_add_constant(xen_chunk* chunk, xen_value value);

#endif
