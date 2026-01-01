#include "xvm.h"
#include "xalloc.h"
#include "xbin_writer.h"
#include "xchunk.h"
#include "xcommon.h"
#include "xerr.h"
#include "xmem.h"
#include "xcompiler.h"
#include "xobject.h"
#include "xstack.h"
#include "xtable.h"
#include "xutils.h"
#include "xvalue.h"
#include "xbuiltin.h"

//====================================================================================================================//

/* Global VM instance */
xen_vm g_vm;

//====================================================================================================================//

static void stack_reset() {
    g_vm.stack_top = g_vm.stack;
}

static void stack_push(xen_value value) {
    *g_vm.stack_top = value;
    g_vm.stack_top++;
}

static xen_value stack_pop() {
    g_vm.stack_top--;
    return *g_vm.stack_top;
}

static xen_value peek(i32 distance) {
    return g_vm.stack_top[-1 - distance];
}

static bool is_falsy(xen_value value) {
    return VAL_IS_NULL(value) || (VAL_IS_BOOL(value) && !VAL_AS_BOOL(value));
}

static void concatenate() {
    const xen_obj_str* b = OBJ_AS_STRING(stack_pop());
    const xen_obj_str* a = OBJ_AS_STRING(stack_pop());
    const i32 length     = a->length + b->length;

    char* str = XEN_ALLOC_ARRAY(g_vm.mem.permanent, char, length + 1);
    memcpy(str, a->str, a->length);
    memcpy(str + a->length, b->str, b->length);
    str[length] = '\0';

    xen_obj_str* result = xen_obj_str_copy(str, length);
    stack_push(OBJ_VAL(result));
}

static void runtime_error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputs("\n", stderr);

    /* print stack trace */
    for (i32 i = g_vm.frame_count - 1; i >= 0; i--) {
        xen_call_frame* frame = &g_vm.frames[i];
        xen_obj_func* fn      = frame->fn;
        size_t instruction    = frame->ip - fn->chunk.code - 1;
        fprintf(stderr, "[line %lu] in ", fn->chunk.lines[instruction]);
        if (fn->name == NULL) {
            fprintf(stderr, "script\n");
        } else {
            fprintf(stderr, "%s()\n", fn->name->str);
        }
    }

    stack_reset();
}

static bool call(xen_obj_func* fn, i32 arg_count) {
    if (arg_count != fn->arity) {
        runtime_error("expected %d arguments but got %d", fn->arity, arg_count);
        return XEN_FALSE;
    }

    if (g_vm.frame_count == FRAMES_MAX) {
        runtime_error("stack overflow");
        return XEN_FALSE;
    }

    xen_call_frame* frame = &g_vm.frames[g_vm.frame_count++];
    frame->fn             = fn;
    frame->ip             = fn->chunk.code;
    frame->slots          = g_vm.stack_top - arg_count - 1;

    return XEN_TRUE;
}

static bool call_value(xen_value callee, i32 arg_count) {
    if (VAL_IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
            case OBJ_FUNCTION:
                return call(OBJ_AS_FUNCTION(callee), arg_count);
            case OBJ_NATIVE_FUNC: {
                xen_native_fn native = OBJ_AS_NATIVE_FUNC(callee)->function;
                xen_value result     = native(arg_count, g_vm.stack_top - arg_count);
                g_vm.stack_top -= arg_count + 1;
                stack_push(result);
                return XEN_TRUE;
            }
            default:
                break;
        }
    }
    runtime_error("can only call functions");
    return XEN_FALSE;
}

//====================================================================================================================//

//====================================================================================================================//

void xen_vm_init(xen_vm_config config) {
    xen_vm_mem_init(&g_vm.mem, config.mem_size_permanent, config.mem_size_generation, config.mem_size_temporary);
    stack_reset();
    g_vm.objects = NULL;
    xen_table_init(&g_vm.globals);
    xen_table_init(&g_vm.strings);
    xen_table_init(&g_vm.namespace_registry);  // register built in namespaces
    xen_builtins_register();
}

void xen_vm_shutdown() {
    xen_table_free(&g_vm.strings);
    xen_table_free(&g_vm.globals);
    xen_table_free(&g_vm.namespace_registry);
    xen_vm_mem_destroy(&g_vm.mem);
}

//====================================================================================================================//

#define READ_BYTE() (*frame->ip++)
#define READ_CONSTANT() (frame->fn->chunk.constants.values[READ_BYTE()])
#define READ_STRING() OBJ_AS_STRING(READ_CONSTANT())
#define BINARY_OP(value_type, op)                                                                                      \
    do {                                                                                                               \
        if (!VAL_IS_NUMBER(peek(0)) || !VAL_IS_NUMBER(peek(1))) {                                                      \
            runtime_error("operands must be numbers");                                                                 \
            return EXEC_RUNTIME_ERROR;                                                                                 \
        }                                                                                                              \
        f64 b = VAL_AS_NUMBER(stack_pop());                                                                            \
        f64 a = VAL_AS_NUMBER(stack_pop());                                                                            \
        stack_push(value_type(a op b));                                                                                \
    } while (XEN_FALSE)
#define READ_SHORT() (frame->ip += 2, (u16)((frame->ip[-2] << 8) | frame->ip[-1]))

//====================================================================================================================//

/* This is the core of the entire interpreter */
static xen_exec_result run() {
    xen_call_frame* frame = &g_vm.frames[g_vm.frame_count - 1];

    for (;;) {
        u8 instruction;
        switch (instruction = READ_BYTE()) {
            case OP_RETURN: {
                xen_value result = stack_pop();
                g_vm.frame_count--;

                if (g_vm.frame_count == 0) {
                    stack_pop();
                    return EXEC_OK;
                }

                g_vm.stack_top = frame->slots;
                stack_push(result);
                frame = &g_vm.frames[g_vm.frame_count - 1];
                break;
            }
            case OP_CONSTANT: {
                const xen_value constant = READ_CONSTANT();
                stack_push(constant);
                break;
            }
            case OP_NULL: {
                stack_push(NULL_VAL);
                break;
            }
            case OP_TRUE: {
                stack_push(BOOL_VAL(XEN_TRUE));
                break;
            }
            case OP_FALSE: {
                stack_push(BOOL_VAL(XEN_FALSE));
                break;
            }  //====================================================================================================================//

            case OP_EQUAL: {
                xen_value b = stack_pop();
                xen_value a = stack_pop();
                stack_push(BOOL_VAL(xen_value_equal(a, b)));
                break;
            }
            case OP_GREATER: {
                BINARY_OP(BOOL_VAL, >);
                break;
            }
            case OP_LESS: {
                BINARY_OP(BOOL_VAL, <);
                break;
            }
            case OP_NEGATE: {
                if (!VAL_IS_NUMBER(peek(0))) {
                    runtime_error("operand must be a number");
                    return EXEC_RUNTIME_ERROR;
                }
                stack_push(NUMBER_VAL(-VAL_AS_NUMBER(stack_pop())));
                break;
            }
            case OP_NOT: {
                stack_push(BOOL_VAL(is_falsy(stack_pop())));
                break;
            }
            case OP_ADD: {
                if (OBJ_IS_STRING(peek(0)) && OBJ_IS_STRING(peek(1))) {
                    concatenate();
                } else if (VAL_IS_NUMBER(peek(0)) && VAL_IS_NUMBER(peek(1))) {
                    const f64 b = VAL_AS_NUMBER(stack_pop());
                    const f64 a = VAL_AS_NUMBER(stack_pop());
                    stack_push(NUMBER_VAL(a + b));
                } else {
                    runtime_error("operands must be of matching types");
                    return EXEC_RUNTIME_ERROR;
                }

                break;
            }
            case OP_SUBTRACT: {
                BINARY_OP(NUMBER_VAL, -);
                break;
            }
            case OP_MULTIPLY: {
                BINARY_OP(NUMBER_VAL, *);
                break;
            }
            case OP_DIVIDE: {
                BINARY_OP(NUMBER_VAL, /);
                break;
            }
            case OP_MOD: {
                if (!VAL_IS_NUMBER(peek(0)) || !VAL_IS_NUMBER(peek(1))) {
                    runtime_error("operands must be numbers");
                    return EXEC_RUNTIME_ERROR;
                }

                f64 b = VAL_AS_NUMBER(stack_pop());
                f64 a = VAL_AS_NUMBER(stack_pop());
                stack_push(NUMBER_VAL(fmod(a, b)));
                break;
            }
            case OP_POP: {
                stack_pop();
                break;
            }
            case OP_PRINT: {
                xen_value_print(stack_pop());
                printf("\n");
                break;
            }
            case OP_GET_LOCAL: {
                u8 slot = READ_BYTE();
                stack_push(frame->slots[slot]);
                break;
            }
            case OP_SET_LOCAL: {
                u8 slot            = READ_BYTE();
                frame->slots[slot] = peek(0);
                break;
            }
            case OP_DEFINE_GLOBAL: {
                xen_obj_str* name = OBJ_AS_STRING(READ_CONSTANT());
                xen_table_set(&g_vm.globals, name, peek(0));
                stack_pop();
                break;
            }
            case OP_GET_GLOBAL: {
                xen_obj_str* name = OBJ_AS_STRING(READ_CONSTANT());
                xen_value value;
                if (!xen_table_get(&g_vm.globals, name, &value)) {
                    runtime_error("undefined variable '%s'", name->str);
                    return EXEC_RUNTIME_ERROR;
                }
                stack_push(value);
                break;
            }
            case OP_SET_GLOBAL: {
                xen_obj_str* name = OBJ_AS_STRING(READ_CONSTANT());
                if (xen_table_set(&g_vm.globals, name, peek(0))) {
                    xen_table_delete(&g_vm.globals, name);
                    runtime_error("undefined variable '%s'", name->str);
                    return EXEC_RUNTIME_ERROR;
                }
                break;
            }
            case OP_CALL: {
                i32 arg_count = READ_BYTE();
                if (!call_value(peek(arg_count), arg_count)) {
                    return EXEC_RUNTIME_ERROR;
                }
                frame = &g_vm.frames[g_vm.frame_count - 1];
                break;
            }
            case OP_JUMP: {
                u16 offset = READ_SHORT();
                frame->ip += offset;
                break;
            }
            case OP_JUMP_IF_FALSE: {
                u16 offset = READ_SHORT();
                if (is_falsy(peek(0))) {
                    frame->ip += offset;
                }
                break;
            }
            case OP_LOOP: {
                u16 offset = READ_SHORT();
                frame->ip -= offset;
                break;
            }
            case OP_INCLUDE: {
                xen_obj_str* name = OBJ_AS_STRING(READ_CONSTANT());
                xen_value namespace_val;
                if (!xen_table_get(&g_vm.namespace_registry, name, &namespace_val)) {
                    runtime_error("unknown namespace '%s'", name->str);
                    return EXEC_RUNTIME_ERROR;
                }
                xen_table_set(&g_vm.globals, name, namespace_val);
                break;
            }
            case OP_GET_PROPERTY: {
                xen_obj_str* name = OBJ_AS_STRING(READ_CONSTANT());
                xen_value obj_val = peek(0);
                if (!VAL_IS_OBJ(obj_val)) {
                    runtime_error("only objects have properties");
                    return EXEC_RUNTIME_ERROR;
                }
                xen_obj* obj = VAL_AS_OBJ(obj_val);
                if (obj->type == OBJ_NAMESPACE) {
                    xen_obj_namespace* ns = (xen_obj_namespace*)obj;
                    xen_value result;
                    if (xen_obj_namespace_get(ns, name->str, &result)) {
                        stack_pop();        /* pop the namespace */
                        stack_push(result); /* push the property value */
                    } else {
                        runtime_error("undefined property '%s' in namespace '%s'");
                        return EXEC_RUNTIME_ERROR;
                    }
                } else {
                    runtime_error("object does not support property access");
                    return EXEC_RUNTIME_ERROR;
                }
                break;
            }
            default: {
                runtime_error("unknown instruction (%d)", instruction);
            }
        }
    }
}

//====================================================================================================================//

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
#undef READ_CONSTANT
#undef READ_STRING

/*
 * Bytecode Binary Format (.xlb)
 *
 * MAGIC             : 'XLBC'    - 4 bytes
 * Version           : u8        - 1 byte
 * Lines             : u32       - 4 bytes
 * Entrypoint Length : u32       - 4 bytes
 * Entrypoint        : <string>  - n bytes
 * Args Count        : u32       - 4 bytes
 * Constants Size    : u32       - 4 bytes
 * Constants Table   : entry[]   - n bytes
 *   Constant Entry
 *     Type          : u8        - 1 byte
 *     Value Length  : u32       - 4 bytes
 *     Value         : <any>     - n bytes
 * Bytecode Size     : u32       - 4 bytes
 * Bytecode          : u8[]      - n bytes
 */

#define WRITE(v) xen_bin_write(&writer, v)

static void write_bytecode(const xen_obj_func* fn, const char* filename) {
    if (!fn) {
        xen_panic(XEN_ERR_INVALID_ARGS, "arg fn is NULL");
    }

    /* write function metadata (constants, line count) */
    xen_bin_writer writer;
    xen_bin_writer_init(&writer, XEN_MB(4));

    const char magic[4]         = {'X', 'L', 'B', 'C'};
    const u8 version            = 1;
    const u32 line_count        = (u32)*fn->chunk.lines;
    const u32 args_count        = (u32)fn->arity;
    const u32 constants_size    = (u32)fn->chunk.constants.count;
    const char* entrypoint_name = fn->name->str;
    const u32 entrypoint_length = (u32)fn->name->length;

    WRITE(magic[0]);
    WRITE(magic[1]);
    WRITE(magic[2]);
    WRITE(magic[3]);
    WRITE(version);
    WRITE(line_count);
    xen_bin_write_fixed_str(&writer, entrypoint_name, entrypoint_length + 1);
    WRITE(args_count);
    WRITE(constants_size);

    /*
     * Constant Entry
     *   Type          : u8        - 1 byte
     *   Value Length  : u32       - 4 bytes
     *   Value         : <any>     - n bytes
     */
    for (int i = 0; i < fn->chunk.constants.count; i++) {
        xen_value constant = fn->chunk.constants.values[i];
        WRITE((u8)constant.type);
        switch (constant.type) {
            case VAL_BOOL: {
                WRITE((u32)sizeof(bool));
                WRITE(constant.as.boolean);
            } break;
            case VAL_NUMBER: {
                WRITE((u32)sizeof(f64));
                WRITE(constant.as.number);
            } break;
            case VAL_OBJECT: {
                if (OBJ_IS_STRING(constant)) {
                    xen_obj_str* str = OBJ_AS_STRING(constant);
                    WRITE((u32)str->length);
                    xen_bin_write_fixed_str(&writer, str->str, strlen(str->str) + 1);
                } else if (OBJ_IS_FUNCTION(constant)) {
                    /* I don't know if I need to write these or not. I don't think I do. */
                } else if (OBJ_IS_NATIVE_FUNC(constant)) {
                }
            } break;
            case VAL_NULL: {
                WRITE((u32)sizeof(bool));
                WRITE(constant.as.boolean);
            } break;
        }
    }

    xen_bin_write_byte_array(&writer, fn->chunk.code, fn->chunk.capacity);

    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        xen_panic(XEN_ERR_OPEN_FILE, "failed to open bytecode file for wriring");
    }

    int result = fwrite(writer.data, writer.consumed, 1, fp);
    if (result == 0) {
        xen_bin_writer_free(&writer);
        fclose(fp);
        xen_panic(XEN_ERR_OPEN_FILE, "failed to write bytecode to file");
    }

    fclose(fp);

    xen_bin_writer_free(&writer);
}

#undef WRITE

xen_exec_result xen_vm_exec(const char* source, bool emit_bytecode, const char* bytecode_filename) {
    xen_obj_func* fn = xen_compile(source);
    if (fn == NULL) {
        return EXEC_COMPILE_ERROR;
    }

    if (emit_bytecode && bytecode_filename) {
        write_bytecode(fn, bytecode_filename);
    }

    stack_push(OBJ_VAL(fn));
    call(fn, 0);

    return run();
}
