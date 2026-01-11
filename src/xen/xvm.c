#include "xvm.h"
#include "xalloc.h"
#include "xchunk.h"
#include "xcommon.h"
#include "xerr.h"
#include "xmem.h"
#include "xcompiler.h"
#include "xstack.h"
#include "xtable.h"
#include "xutils.h"
#include "xvalue.h"
#include "xtypeid.h"
#include "builtin/xbuiltin.h"

#include "object/xobj_string.h"
#include "object/xobj_class.h"
#include "object/xobj_namespace.h"
#include "object/xobj_function.h"
#include "object/xobj_array.h"
#include "object/xobj_dict.h"
#include "object/xobj_instance.h"
#include "object/xobj_bound_method.h"
#include "object/xobj_u8array.h"
#include <string.h>

//====================================================================================================================//

// Global VM instance
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

    // print stack trace
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

static bool is_same_class_context(xen_obj_class* target_class) {
    // Get the current call frame
    xen_call_frame* frame = &g_vm.frames[g_vm.frame_count - 1];

    // Check if we're in a method (slot 0 would be 'this')
    if (frame->slots != NULL) {
        xen_value this_val = frame->slots[0];
        if (OBJ_IS_INSTANCE(this_val)) {
            xen_obj_instance* this_inst = OBJ_AS_INSTANCE(this_val);
            return this_inst->class == target_class;
        }
    }

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
    xen_table_init(&g_vm.namespace_registry);
    xen_table_init(&g_vm.const_globals);
    xen_builtins_register();
}

void xen_vm_shutdown() {
    xen_table_free(&g_vm.strings);
    xen_table_free(&g_vm.globals);
    xen_table_free(&g_vm.namespace_registry);
    xen_table_free(&g_vm.const_globals);
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

// This is the core of the entire interpreter
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
            }
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

                // Check for instance property/method access
                if (OBJ_IS_INSTANCE(obj_val)) {
                    xen_obj_instance* instance = OBJ_AS_INSTANCE(obj_val);
                    xen_value value;

                    // First check fields
                    i32 prop_index = xen_find_property_index(instance->class, name);
                    if (prop_index >= 0) {
                        // check privacy
                        if (instance->class->properties[prop_index].is_private) {
                            if (!is_same_class_context(instance->class)) {
                                runtime_error("cannot access private property '%s'", name->str);
                                return EXEC_RUNTIME_ERROR;
                            }
                        }

                        if (xen_obj_instance_get(instance, name, &value)) {
                            stack_pop();  // Pop instance
                            stack_push(value);
                            break;
                        }
                    }

                    xen_value method;
                    if (xen_table_get(&instance->class->methods, name, &method)) {
                        xen_value receiver = stack_pop();

                        if (OBJ_IS_NATIVE_FUNC(method)) {
                            // Native method - bind it
                            xen_obj_bound_method* bound =
                              xen_obj_bound_method_new(receiver, OBJ_AS_NATIVE_FUNC(method)->function, name->str);
                            stack_push(OBJ_VAL(bound));
                        } else {
                            // Bytecode method
                            xen_obj_bound_method* bound =
                              xen_obj_bound_method_new_func(receiver, OBJ_AS_FUNCTION(method));
                            stack_push(OBJ_VAL(bound));
                        }
                        break;
                    }

                    if (xen_table_get(&instance->class->private_methods, name, &method)) {
                        if (!is_same_class_context(instance->class)) {
                            runtime_error("cannot access private method '%s'", name->str);
                            return EXEC_RUNTIME_ERROR;
                        }
                        xen_value receiver = stack_pop();

                        if (OBJ_IS_NATIVE_FUNC(method)) {
                            xen_obj_bound_method* bound =
                              xen_obj_bound_method_new(receiver, OBJ_AS_NATIVE_FUNC(method)->function, name->str);
                            stack_push(OBJ_VAL(bound));
                        } else {
                            xen_obj_bound_method* bound =
                              xen_obj_bound_method_new_func(receiver, OBJ_AS_FUNCTION(method));
                            stack_push(OBJ_VAL(bound));
                        }
                        break;
                    }

                    runtime_error("undefined property '%s'", name->str);
                    return EXEC_RUNTIME_ERROR;
                }

                // Check for namespace property access
                if (VAL_IS_OBJ(obj_val) && OBJ_TYPE(obj_val) == OBJ_NAMESPACE) {
                    xen_obj_namespace* ns = OBJ_AS_NAMESPACE(obj_val);
                    xen_value result;
                    if (xen_obj_namespace_get(ns, name->str, &result)) {
                        stack_pop();
                        stack_push(result);
                    } else {
                        runtime_error("undefined property '%s' in namespace '%s'", name->str, ns->name);
                        return EXEC_RUNTIME_ERROR;
                    }
                    break;
                }

                // Check for built-in type property/method
                bool is_property     = XEN_FALSE;
                xen_native_fn method = xen_lookup_method(obj_val, name->str, &is_property);

                if (method != NULL) {
                    if (is_property) {
                        // It's a property - call immediately with receiver
                        xen_value receiver = stack_pop();
                        xen_value result   = method(1, &receiver);
                        stack_push(result);
                    } else {
                        // It's a method - create a bound method
                        xen_value receiver          = stack_pop();
                        xen_obj_bound_method* bound = xen_obj_bound_method_new(receiver, method, name->str);
                        stack_push(OBJ_VAL(bound));
                    }
                    break;
                }

                runtime_error("undefined property '%s'", name->str);
                return EXEC_RUNTIME_ERROR;
            }
            case OP_INVOKE: {
                // method invocation: obj.method(args)
                xen_obj_str* method_name = OBJ_AS_STRING(READ_CONSTANT());
                u8 arg_count             = READ_BYTE();

                // the receiver is on the stack below the arguments
                xen_value receiver = peek(arg_count);

                if (OBJ_IS_INSTANCE(receiver)) {
                    xen_obj_instance* instance = OBJ_AS_INSTANCE(receiver);
                    xen_value method;

                    if (xen_table_get(&instance->class->methods, method_name, &method)) {
                        if (OBJ_IS_NATIVE_FUNC(method)) {
                            // Native method - call directly
                            xen_native_fn native = OBJ_AS_NATIVE_FUNC(method)->function;
                            xen_value* args      = g_vm.stack_top - arg_count - 1;
                            xen_value result     = native(arg_count + 1, args);
                            g_vm.stack_top -= arg_count + 1;
                            stack_push(result);
                            break;
                        }

                        // Bytecode method
                        if (!call(OBJ_AS_FUNCTION(method), arg_count)) {
                            return EXEC_RUNTIME_ERROR;
                        }
                        frame = &g_vm.frames[g_vm.frame_count - 1];
                        break;
                    }

                    // Same pattern for private_methods...
                    if (xen_table_get(&instance->class->private_methods, method_name, &method)) {
                        if (!is_same_class_context(instance->class)) {
                            runtime_error("cannot access private method '%s'", method_name->str);
                            return EXEC_RUNTIME_ERROR;
                        }

                        if (OBJ_IS_NATIVE_FUNC(method)) {
                            xen_native_fn native = OBJ_AS_NATIVE_FUNC(method)->function;
                            xen_value* args      = g_vm.stack_top - arg_count - 1;
                            xen_value result     = native(arg_count + 1, args);
                            g_vm.stack_top -= arg_count + 1;
                            stack_push(result);
                            break;
                        }

                        if (!call(OBJ_AS_FUNCTION(method), arg_count)) {
                            return EXEC_RUNTIME_ERROR;
                        }
                        frame = &g_vm.frames[g_vm.frame_count - 1];
                        break;
                    }

                    runtime_error("undefined method '%s'", method_name->str);
                    return EXEC_RUNTIME_ERROR;
                }

                // check for namespace method call
                if (VAL_IS_OBJ(receiver) && OBJ_IS_NAMESPACE(receiver)) {
                    xen_obj_namespace* ns = OBJ_AS_NAMESPACE(receiver);
                    xen_value method_val;
                    if (!xen_obj_namespace_get(ns, method_name->str, &method_val)) {
                        runtime_error("undefined method '%s' in namespace '%s'", method_name->str, ns->name);
                        return EXEC_RUNTIME_ERROR;
                    }

                    // replace namespace on stack with the function, then call
                    g_vm.stack_top[-arg_count - 1] = method_val;
                    if (!call_value(method_val, arg_count)) {
                        return EXEC_RUNTIME_ERROR;
                    }
                    frame = &g_vm.frames[g_vm.frame_count - 1];
                    break;
                }

                // look up built-in method
                bool is_property;
                xen_native_fn method = xen_lookup_method(receiver, method_name->str, &is_property);

                if (method != NULL) {
                    // build args array: [ receiver, arg1, arg2, ... ]
                    xen_value* args  = g_vm.stack_top - arg_count - 1;
                    xen_value result = method(arg_count + 1, args);  // +1 for receiver
                    // pop args and receiver, push result
                    g_vm.stack_top -= arg_count + 1;
                    stack_push(result);
                    break;
                }

                runtime_error("undefined method '%s'", method_name->str);
                return EXEC_RUNTIME_ERROR;
            }
            case OP_ARRAY_NEW: {
                u8 element_count   = READ_BYTE();
                xen_obj_array* arr = xen_obj_array_new_with_capacity(element_count);
                arr->array.count   = element_count;
                // Elements are on stack in reverse order (first pushed = bottom)
                // We need to add them in correct order
                for (i32 i = element_count - 1; i >= 0; i--) {
                    arr->array.values[i] = stack_pop();
                }
                stack_push(OBJ_VAL(arr));
                break;
            }
            case OP_ARRAY_LEN: {
                xen_value array_val = stack_pop();
                if (!OBJ_IS_ARRAY(array_val) && !OBJ_IS_U8ARRAY(array_val)) {
                    runtime_error("can only get length of arrays");
                    return EXEC_RUNTIME_ERROR;
                }

                if (OBJ_IS_ARRAY(array_val)) {
                    xen_obj_array* arr = OBJ_AS_ARRAY(array_val);
                    stack_push(NUMBER_VAL(arr->array.count));
                } else if (OBJ_IS_U8ARRAY(array_val)) {
                    xen_obj_u8array* arr = OBJ_AS_U8ARRAY(array_val);
                    stack_push(NUMBER_VAL(arr->count));
                }

                break;
            }
            case OP_DICT_NEW: {
                stack_push(OBJ_VAL(xen_obj_dict_new()));
                break;
            }
            case OP_DICT_ADD: {
                // stack: [dict, key, value] -> [dict]
                xen_value value    = stack_pop();
                xen_value key      = stack_pop();
                xen_value dict_val = peek(0);  // keep dict on stack for next pair

                if (!OBJ_IS_DICT(dict_val)) {
                    runtime_error("expected dictionary");
                    return EXEC_RUNTIME_ERROR;
                }

                xen_obj_dict* dict = OBJ_AS_DICT(dict_val);
                xen_obj_dict_set(dict, key, value);
                break;
            }
            case OP_INDEX_GET: {
                // stack: [container, index/key] -> [value]
                xen_value index     = stack_pop();
                xen_value container = stack_pop();

                if (OBJ_IS_ARRAY(container)) {
                    if (!VAL_IS_NUMBER(index)) {
                        runtime_error("array index must be a number");
                        return EXEC_RUNTIME_ERROR;
                    }
                    xen_obj_array* arr = OBJ_AS_ARRAY(container);
                    i32 idx            = (i32)VAL_AS_NUMBER(index);

                    if (idx < 0 || idx >= arr->array.count) {
                        runtime_error("array index %d out of bounds (length %d)", idx, arr->array.count);
                        return EXEC_RUNTIME_ERROR;
                    }

                    stack_push(arr->array.values[idx]);
                } else if (OBJ_IS_U8ARRAY(container)) {
                    if (!VAL_IS_NUMBER(index)) {
                        runtime_error("array index must be a number");
                        return EXEC_RUNTIME_ERROR;
                    }
                    xen_obj_u8array* arr = OBJ_AS_U8ARRAY(container);
                    i32 idx              = (i32)VAL_AS_NUMBER(index);

                    if (idx < 0 || idx >= arr->count) {
                        runtime_error("array index %d out of bounds (length %d)", idx, arr->count);
                        return EXEC_RUNTIME_ERROR;
                    }

                    stack_push(NUMBER_VAL(arr->values[idx]));
                } else if (OBJ_IS_DICT(container)) {
                    xen_obj_dict* dict = OBJ_AS_DICT(container);
                    xen_value result;

                    if (!xen_obj_dict_get(dict, index, &result)) {
                        stack_push(NULL_VAL);
                    } else {
                        stack_push(result);
                    }
                } else if (OBJ_IS_STRING(container)) {
                    xen_obj_str* str = OBJ_AS_STRING(container);
                    i32 idx          = (i32)VAL_AS_NUMBER(index);

                    if (idx > str->length - 1) {
                        runtime_error("character index %d out of bounds (length %d)", idx, str->length - 1);
                        return EXEC_RUNTIME_ERROR;
                    }

                    stack_push(OBJ_VAL(xen_obj_str_copy(&str->str[idx], 1)));
                } else {
                    runtime_error("can only index array and dictionaries");
                    return EXEC_RUNTIME_ERROR;
                }

                break;
            }
            case OP_INDEX_SET: {
                // stack: [container, index/key, value] -> [value]
                xen_value value     = stack_pop();
                xen_value index     = stack_pop();
                xen_value container = stack_pop();

                if (OBJ_IS_ARRAY(container)) {
                    // array assignment - index must be a number
                    if (!VAL_IS_NUMBER(index)) {
                        runtime_error("array index must be a number");
                        return EXEC_RUNTIME_ERROR;
                    }
                    xen_obj_array* arr = OBJ_AS_ARRAY(container);
                    i32 idx            = (i32)VAL_AS_NUMBER(index);

                    if (idx < 0 || idx >= arr->array.count) {
                        runtime_error("array index %d out of bounds (length %d)", idx, arr->array.count);
                        return EXEC_RUNTIME_ERROR;
                    }
                    arr->array.values[idx] = value;
                } else if (OBJ_IS_U8ARRAY(container)) {
                    // array assignment - index must be a number
                    if (!VAL_IS_NUMBER(index)) {
                        runtime_error("array index must be a number");
                        return EXEC_RUNTIME_ERROR;
                    }
                    xen_obj_u8array* arr = OBJ_AS_U8ARRAY(container);
                    i32 idx              = (i32)VAL_AS_NUMBER(index);

                    if (idx < 0 || idx >= arr->count) {
                        runtime_error("array index %d out of bounds (length %d)", idx, arr->count);
                        return EXEC_RUNTIME_ERROR;
                    }
                    arr->values[idx] = (u8)VAL_AS_NUMBER(value);
                } else if (OBJ_IS_DICT(container)) {
                    // dictionary assignment - creates or updates key
                    xen_obj_dict* dict = OBJ_AS_DICT(container);
                    xen_obj_dict_set(dict, index, value);

                } else {
                    runtime_error("can only perform index assignments on arrays and dictionaries");
                    return EXEC_RUNTIME_ERROR;
                }

                // assignment is an expression - leave value on stack
                stack_push(value);
                break;
            }
            case OP_CLASS: {
                xen_obj_str* name    = READ_STRING();
                xen_obj_class* class = xen_obj_class_new(name);
                stack_push(OBJ_VAL(class));
                break;
            }
            case OP_PROPERTY: {
                // Stack: [class, default_value]
                u8 name_idx     = READ_BYTE();
                bool is_private = READ_BYTE();

                xen_obj_str* name     = OBJ_AS_STRING(frame->fn->chunk.constants.values[name_idx]);
                xen_value default_val = stack_pop();
                xen_value class_val   = peek(0);

                if (!OBJ_IS_CLASS(class_val)) {
                    runtime_error("can only add properties to classes");
                    return EXEC_RUNTIME_ERROR;
                }

                xen_obj_class* class = OBJ_AS_CLASS(class_val);
                xen_obj_class_add_property(class, name, default_val, is_private);
                break;
            }
            case OP_METHOD: {
                // Stack: [class, function]
                u8 name_idx     = READ_BYTE();
                bool is_private = READ_BYTE();

                xen_obj_str* name    = OBJ_AS_STRING(frame->fn->chunk.constants.values[name_idx]);
                xen_value method_val = stack_pop();
                xen_value class_val  = peek(0);

                xen_obj_class* class = OBJ_AS_CLASS(class_val);
                xen_obj_func* method = OBJ_AS_FUNCTION(method_val);

                xen_obj_class_add_method(class, name, method, is_private);
                break;
            }
            case OP_INITIALIZER: {
                // Stack: [class, function]
                xen_value init_val  = stack_pop();
                xen_value class_val = peek(0);

                xen_obj_class* class = OBJ_AS_CLASS(class_val);
                class->initializer   = OBJ_AS_FUNCTION(init_val);
                break;
            }
            case OP_CALL_INIT: {
                u8 arg_count        = READ_BYTE();
                xen_value class_val = peek(arg_count);

                if (!OBJ_IS_CLASS(class_val)) {
                    runtime_error("can only instantiate classes");
                    return EXEC_RUNTIME_ERROR;
                }

                xen_obj_class* class       = OBJ_AS_CLASS(class_val);
                xen_obj_instance* instance = xen_obj_instance_new(class);

                // Replace class on stack with instance
                g_vm.stack_top[-arg_count - 1] = OBJ_VAL(instance);

                // Check for native initializer first
                if (class->native_initializer != NULL) {
                    // Build args array: [instance, arg1, arg2, ...]
                    xen_value* args  = g_vm.stack_top - arg_count - 1;
                    xen_value result = class->native_initializer(arg_count + 1, args);

                    // Pop arguments, keep instance on stack
                    g_vm.stack_top -= arg_count;

                    if (!OBJ_IS_INSTANCE(result) && !VAL_IS_NULL(result)) {
                        xen_runtime_error("native initializer for class '%s' returned invalid type: %d",
                                          class->name->str,
                                          xen_typeid_get(result));
                        return EXEC_RUNTIME_ERROR;
                    }
                }
                // Call initializer if present
                else if (class->initializer != NULL) {
                    if (!call(class->initializer, arg_count)) {
                        return EXEC_RUNTIME_ERROR;
                    }
                    frame = &g_vm.frames[g_vm.frame_count - 1];
                } else if (arg_count != 0) {
                    runtime_error("expected 0 arguments but got %d", arg_count);
                    return EXEC_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SET_PROPERTY: {
                // Stack: [instance, value]
                xen_obj_str* name = READ_STRING();

                xen_value value    = stack_pop();
                xen_value inst_val = peek(0);

                if (!OBJ_IS_INSTANCE(inst_val)) {
                    runtime_error("only instances have properties");
                    return EXEC_RUNTIME_ERROR;
                }

                xen_obj_instance* instance = OBJ_AS_INSTANCE(inst_val);

                if (xen_obj_class_is_property_private(instance->class, name)) {
                    if (!is_same_class_context(instance->class)) {
                        runtime_error("cannot access private property '%s'", name->str);
                        return EXEC_RUNTIME_ERROR;
                    }
                }

                if (!xen_obj_instance_set(instance, name, value)) {
                    runtime_error("undefined property '%s'", name->str);
                    return EXEC_RUNTIME_ERROR;
                }

                // Pop instance, push value (assignment expression returns value)
                stack_pop();
                stack_push(value);
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

static xen_exec_result exec(xen_obj_func* fn) {
    if (fn == NULL) {
        return EXEC_COMPILE_ERROR;
    }

    stack_push(OBJ_VAL(fn));
    call(fn, 0);

    return run();
}

typedef struct {
    u8 typeid;

    union {
        bool b;
        f64 n;

        struct str {
            i32 len;
            char* chars;
        } str;
    } as;
} xen_constant_value;

/*
 * Bytecode Binary Format (.xenb):
 *
 * MAGIC             : 'XENB'    - 4 bytes
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
 *
 * ---------------------------------------
 *
 * Constant Entry
 *   Type          : u8        - 1 byte
 *   Value Length  : u32       - 4 bytes
 *   Value         : <any>     - n bytes
 *
 */
xen_obj_func* xen_decode_bytecode(u8* bytecode, const size_t size) {
    xen_obj_func* fn = xen_obj_func_new();

    i32 offset    = 0;
    char magic[5] = {'\0'};
    memcpy(magic, bytecode, 4);
    if (strcmp(magic, "XENB") != 0) {
        fprintf(stderr, "Invalid bytecode format\n");
        return NULL;
    }
    offset += 4;

    u8 version = 0;
    memcpy(&version, bytecode + offset, sizeof(u8));
    if (version != 1) {
        fprintf(stderr, "Incorrect version: %d (expected: 1)\n", (i32)version);
        return NULL;
    }
    offset += sizeof(u8);

    u32 lines;
    memcpy(&lines, bytecode + offset, sizeof(u32));
    offset += sizeof(u32);

    u32 entrypoint_length;
    memcpy(&entrypoint_length, bytecode + offset, sizeof(u32));
    offset += sizeof(u32);

    char* entrypoint_name = (char*)malloc((entrypoint_length + 1) * sizeof(char));
    if (!entrypoint_name) {
        fprintf(stderr, "failed to allocate entrypoint string with length: %d\n", entrypoint_length);
        return NULL;
    }
    memcpy(entrypoint_name, bytecode + offset, entrypoint_length);
    entrypoint_name[entrypoint_length] = '\0';
    offset += entrypoint_length;

    u32 args_count;
    memcpy(&args_count, bytecode + offset, sizeof(u32));
    offset += sizeof(u32);

    u32 constants_size;
    memcpy(&constants_size, bytecode + offset, sizeof(u32));
    offset += sizeof(u32);

    // Read contents table
    /*
     * Constant Entry
     *   Type          : u8        - 1 byte
     *   Value Length  : u32       - 4 bytes
     *   Value         : <any>     - n bytes
     */
    xen_constant_value* constants = (xen_constant_value*)malloc(constants_size);
    for (u32 i = 0; i < constants_size; i++) {
        // track how many bytes we've read for this entry
        i32 read = 0;

        u8 type;
        memcpy(&type, bytecode + offset, sizeof(u8));
        offset += sizeof(u8);
        read += sizeof(u8);

        u32 value_length;
        memcpy(&type, bytecode + offset, sizeof(u8));
        offset += sizeof(u8);
        read += sizeof(u8);

        switch (type) {
            case TYPEID_BOOL: {
                bool val;
                memcpy(&val, bytecode + offset, sizeof(bool));
                constants[i].typeid = type;
                constants[i].as.b   = val;
                offset += value_length;
                read += value_length;
            } break;
            case TYPEID_NUMBER: {
                f64 val;
                memcpy(&val, bytecode + offset, sizeof(f64));
                constants[i].typeid = type;
                constants[i].as.n   = val;
                offset += value_length;
                read += value_length;
            } break;
            case TYPEID_STRING: {
                u32 len;
                memcpy(&len, bytecode + offset, sizeof(u32));
                offset += sizeof(u32);
                read += sizeof(u32);

                char* val = (char*)malloc(len + 1);
                memcpy(val, bytecode + offset, len);
                val[len] = '\0';

                constants[i].typeid       = type;
                constants[i].as.str.len   = 0;
                constants[i].as.str.chars = xen_strdup(val);
                offset += value_length;
                read += value_length;

                free(val);
            } break;
            case TYPEID_NULL: {
                bool val;
                memcpy(&val, bytecode + offset, sizeof(bool));
                constants[i].typeid = type;
                constants[i].as.b   = val;
                offset += value_length;
                read += value_length;
            } break;
        }

        // move to next entry
        offset += read;
    }

    u32 bytecode_size;
    memcpy(&bytecode_size, bytecode + offset, sizeof(u32));
    offset += sizeof(u32);

    u8* bytecode_bytes = NULL;
    memcpy(bytecode_bytes, bytecode + offset, bytecode_size);

    //  * MAGIC             : 'XENB'    - 4 bytes
    //  * Version           : u8        - 1 byte
    //  * Lines             : u32       - 4 bytes
    //  * Entrypoint Length : u32       - 4 bytes
    //  * Entrypoint        : <string>  - n bytes
    //  * Args Count        : u32       - 4 bytes
    //  * Constants Size    : u32       - 4 bytes
    //  * Constants Table   : entry[]   - n bytes
    //  *   Constant Entry
    //  *     Type          : u8        - 1 byte
    //  *     Value Length  : u32       - 4 bytes
    //  *     Value         : <any>     - n bytes
    //  * Bytecode Size     : u32       - 4 bytes
    //  * Bytecode          : u8[]      - n bytes

    printf("magic: %s\n", magic);
    printf("version: %d\n", version);
    printf("lines: %d\n", lines);
    printf("entrypoint: %s\n", entrypoint_name);
    printf("args count: %d\n", args_count);
    printf("constants count: %d\n", constants_size);
    printf("bytecode size: %d\n", bytecode_size);

    free(entrypoint_name);

    return fn;
}

xen_exec_result xen_vm_exec(const char* source) {
    xen_obj_func* fn = xen_compile(source);
    return exec(fn);
}

xen_exec_result xen_vm_exec_bytecode(u8* bytecode, const size_t size) {
    xen_obj_func* fn = xen_decode_bytecode(bytecode, size);
    return exec(fn);
}