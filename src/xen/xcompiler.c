#include "xcompiler.h"
#include "xchunk.h"
#include "xcommon.h"
#include "xerr.h"
#include "xscanner.h"
#include "xvalue.h"
#include "xtable.h"
#include "xvm.h"
#include "xutils.h"
#include "builtin/xbuiltin.h"

// objects
#include "object/xobj_function.h"
#include "object/xobj_string.h"

#include <math.h>
#include <string.h>

// ============================================================================
// Types
// ============================================================================

typedef struct {
    xen_token current;
    xen_token previous;
    bool had_error;
    bool panic_mode;
    // remember last variable parsed for postfix operators
    bool last_was_variable;
    xen_token last_variable;
    bool last_was_local;
    i32 last_variable_arg;
} xen_parser;

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -
    PREC_FACTOR,      // * /
    PREC_UNARY,       // ! -
    PREC_CALL,        // . ()
    PREC_POSTFIX,     // ++ -- (postfix)
    PREC_PRIMARY
} xen_op_prec;

typedef void (*xen_parse_fn)(bool can_assign);

typedef struct {
    xen_parse_fn prefix;
    xen_parse_fn infix;
    xen_op_prec precedence;
} xen_parse_rule;

typedef struct {
    xen_token name;
    i32 depth;
    bool is_const;
} xen_local;

typedef enum {
    TYPE_FUNCTION,
    TYPE_SCRIPT,
    TYPE_METHOD,
    TYPE_INITIALIZER,
} xen_function_type;

#define MAX_LOCALS 256

typedef struct xen_compiler {
    struct xen_compiler* enclosing;
    xen_obj_func* function;
    xen_function_type type;

    xen_local locals[MAX_LOCALS];
    i32 local_count;
    i32 scope_depth;
} xen_compiler;

typedef struct class_compiler {
    struct class_compiler* enclosing;
    xen_token name;
} class_compiler;

static class_compiler* current_class = NULL;

static bool is_valid_namespace(const char* name, i32 length) {
    for (i32 i = 0; xen_builtin_namespaces[i] != NULL; i++) {
        if ((i32)strlen(xen_builtin_namespaces[i]) == length && memcmp(xen_builtin_namespaces[i], name, length) == 0) {
            return XEN_TRUE;
        }
    }
    return XEN_FALSE;
}

#define MAX_INCLUDES 64
#define MAX_PATH_LENGTH 256

typedef struct {
    char* paths[MAX_INCLUDES];
    i32 count;
} xen_include_tracker;

typedef struct {
    char path[MAX_PATH_LENGTH];
    char* source;
    xen_token_scanner scanner;
    xen_token current;
    xen_token previous;
} xen_compile_context;

// ============================================================================
// Globals
// ============================================================================

xen_parser parser;
xen_compiler* current = NULL;
static xen_include_tracker include_tracker;
static char current_file_path[MAX_PATH_LENGTH] = ".";

// ============================================================================
// Forward Declarations
// ============================================================================

static void expression();
static void statement();
static void declaration();
static xen_parse_rule* get_rule(xen_token_type);
static void parse_precedence(xen_op_prec);
static void compile_file_include(const char* path, const char* namespace_name);
static char* load_file_source(const char* path);

// ============================================================================
// Helpers
// ============================================================================

static xen_chunk* current_chunk() {
    return &current->function->chunk;
}

static void error_at(const xen_token* token, const char* msg) {
    if (parser.panic_mode)
        return;

    parser.panic_mode = XEN_TRUE;
    fprintf(stderr, "[line %d] error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        // Nothing
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", msg);
    parser.had_error = XEN_TRUE;
}

static void error(const char* msg) {
    error_at(&parser.previous, msg);
}

static void error_at_current(const char* msg) {
    error_at(&parser.current, msg);
}

static void advance() {
    parser.previous = parser.current;

    for (;;) {
        parser.current = xen_scanner_emit();
        if (parser.current.type != TOKEN_ERROR) {
            break;
        }

        error_at_current(parser.current.start);
    }
}

static void consume(const xen_token_type type, const char* msg) {
    if (parser.current.type == type) {
        advance();
        return;
    }

    error_at_current(msg);
}

static bool check(xen_token_type type) {
    return parser.current.type == type;
}

static bool match_token(xen_token_type type) {
    if (!check(type))
        return XEN_FALSE;
    advance();
    return XEN_TRUE;
}

static void include_tracker_init() {
    include_tracker.count = 0;
    for (i32 i = 0; i < MAX_INCLUDES; i++) {
        include_tracker.paths[i] = NULL;
    }
}

static void include_tracker_cleanup() {
    for (i32 i = 0; i < include_tracker.count; i++) {
        free(include_tracker.paths[i]);
        include_tracker.paths[i] = NULL;
    }
    include_tracker.count = 0;
}

static bool is_already_included(const char* path) {
    for (i32 i = 0; i < include_tracker.count; i++) {
        if (strcmp(include_tracker.paths[i], path) == 0) {
            return XEN_TRUE;
        }
    }
    return XEN_FALSE;
}

static void mark_as_included(const char* path) {
    if (include_tracker.count >= MAX_INCLUDES) {
        error("too many includes");
        return;
    }
    include_tracker.paths[include_tracker.count] = xen_strdup((char*)path);
    include_tracker.count++;
}

static void get_directory(const char* filepath, char* dir, size_t dir_size) {
    strncpy(dir, filepath, dir_size - 1);
    dir[dir_size - 1] = '\0';

    char* last_slash = strrchr(dir, '/');
#ifdef _WIN32
    char* last_backslash = strrchr(dir, '\\');
    if (last_backslash > last_slash)
        last_slash = last_backslash;
#endif

    if (last_slash) {
        *(last_slash + 1) = '\0';
    } else {
        strcpy(dir, "./");
    }
}

static void resolve_include_path(const char* include_name, char* out_path, size_t out_size) {
    char dir[MAX_PATH_LENGTH];
    get_directory(current_file_path, dir, sizeof(dir));
    snprintf(out_path, out_size, "%s%s.xen", dir, include_name);
}

// ============================================================================
// Emit Bytecode
// ============================================================================

static void emit_byte(const u8 byte) {
    xen_chunk_write(current_chunk(), byte, parser.previous.line);
}

static void emit_bytes(const u8 op, const u8 operand) {
    emit_byte(op);
    emit_byte(operand);
}

static void emit_return() {
    if (current->type == TYPE_INITIALIZER) {
        emit_bytes(OP_GET_LOCAL, 0);
    } else {
        emit_byte(OP_NULL);
    }
    emit_byte(OP_RETURN);
}

static i32 emit_jump(u8 instruction) {
    emit_byte(instruction);
    emit_byte(0xFF);                    // placeholder (will be patched)
    emit_byte(0xFF);                    // placeholder (will be patched)
    return current_chunk()->count - 2;  // return offset location
}

static void patch_jump(i32 offset) {
    i32 jump = current_chunk()->count - offset - 2;
    if (jump > UINT16_MAX) {
        error("too much code to jump over");
    }
    current_chunk()->code[offset]     = (jump >> 8) & 0xFF;
    current_chunk()->code[offset + 1] = jump & 0xFF;
}

static void emit_loop(i32 loop_start) {
    emit_byte(OP_LOOP);
    i32 offset = current_chunk()->count - loop_start + 2;
    if (offset > UINT16_MAX) {
        error("loop body too large");
    }
    emit_byte((offset >> 8) & 0xFF);
    emit_byte(offset & 0xFF);
}

static u8 make_constant(xen_value value) {
    const i32 constant = xen_chunk_add_constant(current_chunk(), value);
    if (constant > UINT8_MAX) {
        error("too many constants in one chunk");
        return 0;
    }
    return (u8)constant;
}

static void emit_constant(xen_value value) {
    emit_bytes(OP_CONSTANT, make_constant(value));
}

static xen_obj_func* end_compiler() {
    emit_return();
    xen_obj_func* fn = current->function;
    current          = current->enclosing;
    return fn;
}

// ============================================================================
// Compiler State
// ============================================================================

static void init_compiler(xen_compiler* compiler, xen_function_type type) {
    compiler->enclosing   = current;
    compiler->function    = NULL;
    compiler->type        = type;
    compiler->local_count = 0;
    compiler->scope_depth = 0;
    compiler->function    = xen_obj_func_new();
    current               = compiler;

    if (type != TYPE_SCRIPT) {
        current->function->name = xen_obj_str_copy(parser.previous.start, parser.previous.length);
    }

    // First slot is reserved for the function itself (or "this" in methods)
    xen_local* local   = &current->locals[current->local_count++];
    local->depth       = 0;
    local->name.start  = "";
    local->name.length = 0;
    local->is_const    = XEN_FALSE;

    if (type == TYPE_METHOD || type == TYPE_INITIALIZER) {
        // 'this' occupies slot 0
        local->name.start  = "this";
        local->name.length = 4;
    } else {
        local->name.start  = "";
        local->name.length = 0;
    }
}

// ============================================================================
// Scope Management
// ============================================================================

static void begin_scope() {
    current->scope_depth++;
}

static void end_scope() {
    current->scope_depth--;

    // Pop all locals in this scope
    while (current->local_count > 0 && current->locals[current->local_count - 1].depth > current->scope_depth) {
        emit_byte(OP_POP);
        current->local_count--;
    }
}

// ============================================================================
// Variables
// ============================================================================

static u8 identifier_constant(xen_token* name) {
    return make_constant(OBJ_VAL(xen_obj_str_copy(name->start, name->length)));
}

static bool identifiers_equal(xen_token* a, xen_token* b) {
    if (a->length != b->length)
        return XEN_FALSE;
    return memcmp(a->start, b->start, a->length) == 0;
}

static i32 resolve_local(xen_compiler* compiler, xen_token* name) {
    for (i32 i = compiler->local_count - 1; i >= 0; i--) {
        xen_local* local = &compiler->locals[i];
        if (identifiers_equal(name, &local->name)) {
            if (local->depth == -1) {
                error("cannot read local variable in its own initializer");
            }
            return i;
        }
    }
    return -1;
}

static void add_local(xen_token name, bool is_const) {
    if (current->local_count == MAX_LOCALS) {
        error("too many local variables in function");
        return;
    }

    xen_local* local = &current->locals[current->local_count++];
    local->name      = name;
    local->depth     = -1;  // Mark as uninitialized
    local->is_const  = is_const;
}

static void declare_variable() {
    if (current->scope_depth == 0)
        return;

    xen_token* name = &parser.previous;

    // Check for existing variable in current scope
    for (i32 i = current->local_count - 1; i >= 0; i--) {
        xen_local* local = &current->locals[i];
        if (local->depth != -1 && local->depth < current->scope_depth) {
            break;
        }

        if (identifiers_equal(name, &local->name)) {
            error("already a variable with this name in this scope");
        }
    }

    add_local(*name, XEN_FALSE);
}

static u8 parse_variable(const char* msg) {
    consume(TOKEN_IDENTIFIER, msg);

    declare_variable();
    if (current->scope_depth > 0)
        return 0;

    return identifier_constant(&parser.previous);
}

static void mark_initialized() {
    if (current->scope_depth == 0)
        return;
    current->locals[current->local_count - 1].depth = current->scope_depth;
}

static bool is_local_const(xen_compiler* compiler, i32 slot) {
    if (slot < 0 || slot >= compiler->local_count) {
        return XEN_FALSE;
    }
    return compiler->locals[slot].is_const;
}

static bool is_global_const(xen_token* name) {
    xen_obj_str* name_str = xen_obj_str_copy(name->start, name->length);
    xen_value dummy;
    bool result = xen_table_get(&g_vm.const_globals, name_str, &dummy);
    return result;
}

static void mark_global_const(xen_token* name) {
    xen_obj_str* name_str = xen_obj_str_copy(name->start, name->length);
    xen_table_set(&g_vm.const_globals, name_str, BOOL_VAL(XEN_TRUE));
}

static void define_variable(u8 global) {
    if (current->scope_depth > 0) {
        mark_initialized();
        return;
    }

    emit_bytes(OP_DEFINE_GLOBAL, global);
}

static void named_variable(xen_token name, bool can_assign) {
    u8 get_op, set_op;
    i32 arg = resolve_local(current, &name);

    bool is_local     = (arg != -1);
    bool is_const_var = XEN_FALSE;

    if (is_local) {
        is_const_var = is_local_const(current, arg);
        get_op       = OP_GET_LOCAL;
        set_op       = OP_SET_LOCAL;
    } else {
        is_const_var = is_global_const(&name);
        arg          = identifier_constant(&name);
        get_op       = OP_GET_GLOBAL;
        set_op       = OP_SET_GLOBAL;
    }

    // record for potential postfix operator
    parser.last_was_variable = XEN_TRUE;
    parser.last_variable     = name;
    parser.last_was_local    = is_local;
    parser.last_variable_arg = arg;

    // check for any assignment to const variable
    if (can_assign && is_const_var) {
        if (check(TOKEN_EQUAL) || check(TOKEN_PLUS_EQUAL) || check(TOKEN_MINUS_EQUAL) || check(TOKEN_ASTERISK_EQUAL) ||
            check(TOKEN_SLASH_EQUAL) || check(TOKEN_PERCENT_EQUAL)) {
            error("cannot assign to constant variable");
            // Consume the token to prevent cascading errors
            advance();
            if (!check(TOKEN_SEMICOLON)) {
                expression();
            }
            return;
        }
    }

    if (can_assign && match_token(TOKEN_EQUAL)) {
        parser.last_was_variable = XEN_FALSE;  // consumed by assignment
        expression();
        emit_bytes(set_op, (u8)arg);
    } else if (can_assign && match_token(TOKEN_PLUS_EQUAL)) {
        // i += expr  →  i = i + expr
        parser.last_was_variable = XEN_FALSE;
        emit_bytes(get_op, (u8)arg);  // push current value
        expression();                 // push increment
        emit_byte(OP_ADD);            // add them
        emit_bytes(set_op, (u8)arg);  // store result
    } else if (can_assign && match_token(TOKEN_MINUS_EQUAL)) {
        parser.last_was_variable = XEN_FALSE;
        emit_bytes(get_op, (u8)arg);
        expression();
        emit_byte(OP_SUBTRACT);
        emit_bytes(set_op, (u8)arg);
    } else if (can_assign && match_token(TOKEN_ASTERISK_EQUAL)) {
        parser.last_was_variable = XEN_FALSE;
        emit_bytes(get_op, (u8)arg);
        expression();
        emit_byte(OP_MULTIPLY);
        emit_bytes(set_op, (u8)arg);
    } else if (can_assign && match_token(TOKEN_SLASH_EQUAL)) {
        parser.last_was_variable = XEN_FALSE;
        emit_bytes(get_op, (u8)arg);
        expression();
        emit_byte(OP_DIVIDE);
        emit_bytes(set_op, (u8)arg);
    } else if (can_assign && match_token(TOKEN_PERCENT_EQUAL)) {
        parser.last_was_variable = XEN_FALSE;
        emit_bytes(get_op, (u8)arg);
        expression();
        emit_byte(OP_MOD);
        emit_bytes(set_op, (u8)arg);
    } else {
        emit_bytes(get_op, (u8)arg);
    }
}

// ============================================================================
// Expression Parsing
// ============================================================================

static void grouping(bool can_assign) {
    XEN_UNUSED(can_assign);
    expression();
    consume(TOKEN_RIGHT_PAREN, "expected closing ')'");
}

static void number(bool can_assign) {
    XEN_UNUSED(can_assign);
    f64 value = strtod(parser.previous.start, NULL);
    emit_constant(NUMBER_VAL(value));
}

static void string(bool can_assign) {
    XEN_UNUSED(can_assign);
    emit_constant(OBJ_VAL(xen_obj_str_copy(parser.previous.start + 1, parser.previous.length - 2)));
}

static void variable(bool can_assign) {
    named_variable(parser.previous, can_assign);
}

static void unary(bool can_assign) {
    XEN_UNUSED(can_assign);
    const xen_token_type op_type = parser.previous.type;
    parse_precedence(PREC_UNARY);

    switch (op_type) {
        case TOKEN_MINUS:
            emit_byte(OP_NEGATE);
            break;
        case TOKEN_BANG:
            emit_byte(OP_NOT);
            break;
        default:
            return;
    }
}

static void binary(bool can_assign) {
    XEN_UNUSED(can_assign);
    const xen_token_type op_type = parser.previous.type;
    xen_parse_rule* rule         = get_rule(op_type);
    parse_precedence((xen_op_prec)(rule->precedence + 1));

    switch (op_type) {
        case TOKEN_BANG_EQUAL:
            emit_bytes(OP_EQUAL, OP_NOT);
            break;
        case TOKEN_EQUAL_EQUAL:
            emit_byte(OP_EQUAL);
            break;
        case TOKEN_GREATER:
            emit_byte(OP_GREATER);
            break;
        case TOKEN_GREATER_EQUAL:
            emit_bytes(OP_LESS, OP_NOT);
            break;
        case TOKEN_LESS:
            emit_byte(OP_LESS);
            break;
        case TOKEN_LESS_EQUAL:
            emit_bytes(OP_GREATER, OP_NOT);
            break;
        case TOKEN_PLUS:
            emit_byte(OP_ADD);
            break;
        case TOKEN_MINUS:
            emit_byte(OP_SUBTRACT);
            break;
        case TOKEN_ASTERISK:
            emit_byte(OP_MULTIPLY);
            break;
        case TOKEN_PERCENT:
            emit_byte(OP_MOD);
            break;
        case TOKEN_FORWARD_SLASH:
            emit_byte(OP_DIVIDE);
            break;
        default:
            return;
    }
}

static void literal(bool can_assign) {
    XEN_UNUSED(can_assign);
    switch (parser.previous.type) {
        case TOKEN_FALSE:
            emit_byte(OP_FALSE);
            break;
        case TOKEN_NULL:
            emit_byte(OP_NULL);
            break;
        case TOKEN_TRUE:
            emit_byte(OP_TRUE);
            break;
        default:
            return;
    }
}

static u8 argument_list() {
    u8 arg_count = 0;
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            expression();
            if (arg_count == 255) {
                error("cannot have more than 255 arguments");
            }
            arg_count++;
        } while (match_token(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "expected ')' after arguments");
    return arg_count;
}

static void call(bool can_assign) {
    XEN_UNUSED(can_assign);
    u8 arg_count = argument_list();
    emit_bytes(OP_CALL, arg_count);
}

static void and_(bool can_assign) {
    XEN_UNUSED(can_assign);
    // left operand already compiled and on stack
    i32 end_jump = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP);           // discard left if truthy
    parse_precedence(PREC_AND);  // compile right operand
    patch_jump(end_jump);
    // if left was false, it's still on stack (short-circuit)
    // if left was true, right operand is on stack
}

static void or_(bool can_assign) {
    XEN_UNUSED(can_assign);
    // jump over the "skip right side" jump if false
    i32 else_jump = emit_jump(OP_JUMP_IF_FALSE);
    i32 end_jump  = emit_jump(OP_JUMP);
    patch_jump(else_jump);
    emit_byte(OP_POP);
    parse_precedence(PREC_OR);
    patch_jump(end_jump);
}

static void postfix_inc(bool can_assign) {
    XEN_UNUSED(can_assign);

    if (!parser.last_was_variable) {
        error("operand of '++' must be a variable");
        return;
    }

    bool is_const_var = XEN_FALSE;
    if (parser.last_was_local) {
        is_const_var = is_local_const(current, parser.last_variable_arg);
    } else {
        is_const_var = is_global_const(&parser.last_variable);
    }
    if (is_const_var) {
        error("cannot modify constant variable with '++'");
        parser.last_was_variable = XEN_FALSE;
        return;
    }

    u8 get_op = parser.last_was_local ? OP_GET_LOCAL : OP_GET_GLOBAL;
    u8 set_op = parser.last_was_local ? OP_SET_LOCAL : OP_SET_GLOBAL;
    u8 arg    = (u8)parser.last_variable_arg;

    // Stack currently has [old_value] from the variable() call
    // We need: [old_value] but also increment the variable

    emit_bytes(get_op, arg);       // [old, old]  (re-fetch, we'll increment this)
    emit_constant(NUMBER_VAL(1));  // [old, old, 1]
    emit_byte(OP_ADD);             // [old, new]
    emit_bytes(set_op, arg);       // [old, new]
    emit_byte(OP_POP);             // [old]

    parser.last_was_variable = XEN_FALSE;
}

static void postfix_dec(bool can_assign) {
    XEN_UNUSED(can_assign);

    if (!parser.last_was_variable) {
        error("operand of '--' must be a variable");
        return;
    }

    bool is_const_var = XEN_FALSE;
    if (parser.last_was_local) {
        is_const_var = is_local_const(current, parser.last_variable_arg);
    } else {
        is_const_var = is_global_const(&parser.last_variable);
    }
    if (is_const_var) {
        error("cannot modify constant variable with '--'");
        parser.last_was_variable = XEN_FALSE;
        return;
    }

    u8 get_op = parser.last_was_local ? OP_GET_LOCAL : OP_GET_GLOBAL;
    u8 set_op = parser.last_was_local ? OP_SET_LOCAL : OP_SET_GLOBAL;
    u8 arg    = (u8)parser.last_variable_arg;

    emit_bytes(get_op, arg);
    emit_constant(NUMBER_VAL(1));
    emit_byte(OP_SUBTRACT);
    emit_bytes(set_op, arg);
    emit_byte(OP_POP);

    parser.last_was_variable = XEN_FALSE;
}

static void prefix_inc(bool can_assign) {
    XEN_UNUSED(can_assign);

    // The ++ has been consumed, next token should be identifier
    consume(TOKEN_IDENTIFIER, "expected variable after '++'");

    xen_token name = parser.previous;
    u8 get_op, set_op;
    i32 arg = resolve_local(current, &name);

    bool is_local     = (arg != -1);
    bool is_const_var = XEN_FALSE;
    if (is_local) {
        is_const_var = is_local_const(current, arg);
    } else {
        is_const_var = is_global_const(&name);
    }
    if (is_const_var) {
        error("cannot modify constant variable with '++'");
        return;
    }

    if (is_local) {
        get_op = OP_GET_LOCAL;
        set_op = OP_SET_LOCAL;
    } else {
        arg    = identifier_constant(&name);
        get_op = OP_GET_GLOBAL;
        set_op = OP_SET_GLOBAL;
    }

    // ++i: increment first, return new value
    emit_bytes(get_op, (u8)arg);   // [old]
    emit_constant(NUMBER_VAL(1));  // [old, 1]
    emit_byte(OP_ADD);             // [new]
    emit_bytes(set_op, (u8)arg);   // [new] (stored and left on stack)
}

static void prefix_dec(bool can_assign) {
    XEN_UNUSED(can_assign);

    consume(TOKEN_IDENTIFIER, "expected variable after '--'");

    xen_token name = parser.previous;
    u8 get_op, set_op;
    i32 arg = resolve_local(current, &name);

    bool is_local     = (arg != -1);
    bool is_const_var = XEN_FALSE;
    if (is_local) {
        is_const_var = is_local_const(current, arg);
    } else {
        is_const_var = is_global_const(&name);
    }
    if (is_const_var) {
        error("cannot modify constant variable with '--'");
        return;
    }

    if (is_local) {
        get_op = OP_GET_LOCAL;
        set_op = OP_SET_LOCAL;
    } else {
        arg    = identifier_constant(&name);
        get_op = OP_GET_GLOBAL;
        set_op = OP_SET_GLOBAL;
    }

    emit_bytes(get_op, (u8)arg);
    emit_constant(NUMBER_VAL(1));
    emit_byte(OP_SUBTRACT);
    emit_bytes(set_op, (u8)arg);
}

static void dot(bool can_assign) {
    consume(TOKEN_IDENTIFIER, "expected property name after '.'");
    u8 name = identifier_constant(&parser.previous);

    if (can_assign && match_token(TOKEN_EQUAL)) {
        // Property assignment: obj.prop = value
        expression();
        emit_bytes(OP_SET_PROPERTY, name);
    } else if (match_token(TOKEN_LEFT_PAREN)) {
        // Method call: obj.method(args)
        u8 arg_count = argument_list();
        emit_bytes(OP_INVOKE, name);
        emit_byte(arg_count);
    } else {
        // Property access: obj.prop
        emit_bytes(OP_GET_PROPERTY, name);
    }
}

static void array_lit(bool can_assign) {
    XEN_UNUSED(can_assign);
    u8 element_count = 0;
    if (!check(TOKEN_RIGHT_BRACKET)) {
        do {
            if (element_count == 255) {
                error("cannot have more than 255 elements in array literal");
            }
            expression();
            element_count++;
        } while (match_token(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_BRACKET, "expected ']' after array elements");
    emit_bytes(OP_ARRAY_NEW, element_count);
}

static void subscript(bool can_assign) {
    expression();
    consume(TOKEN_RIGHT_BRACKET, "expected ']' after index");
    if (can_assign && match_token(TOKEN_EQUAL)) {
        expression();
        emit_byte(OP_INDEX_SET);
    } else {
        emit_byte(OP_INDEX_GET);
    }
}

static void function(xen_function_type type);

static void fn_expr(bool can_assign) {
    XEN_UNUSED(can_assign);
    function(TYPE_FUNCTION);
}

static void dictionary(bool can_assign) {
    XEN_UNUSED(can_assign);
    emit_byte(OP_DICT_NEW);

    if (!check(TOKEN_RIGHT_BRACE)) {
        do {
            if (!check(TOKEN_STRING)) {
                error("dictionary key must be a string");
                return;
            }
            expression();  // parse string
            consume(TOKEN_COLON, "expect ':' after dictionary key");
            expression();  // parse value
            emit_byte(OP_DICT_ADD);
        } while (match_token(TOKEN_COMMA));
    }

    consume(TOKEN_RIGHT_BRACE, "expect '}' after dictionary");
}

static void this_(bool can_assign) {
    XEN_UNUSED(can_assign);

    if (current_class == NULL) {
        error("cannot use 'this' outside of a class body");
        return;
    }

    // 'this' is always local slot 0 in methods
    variable(XEN_FALSE);
}

static void new_expr(bool can_assign) {
    XEN_UNUSED(can_assign);

    consume(TOKEN_IDENTIFIER, "expect class name after 'new'");
    u8 name_constant = identifier_constant(&parser.previous);
    emit_bytes(OP_GET_GLOBAL, name_constant);

    // Handle dotted access (e.g., net.TCPListener)
    while (match_token(TOKEN_DOT)) {
        consume(TOKEN_IDENTIFIER, "expect property name after '.'");
        u8 prop_constant = identifier_constant(&parser.previous);
        emit_bytes(OP_GET_PROPERTY, prop_constant);
    }

    consume(TOKEN_LEFT_PAREN, "expect '(' after class name");
    u8 arg_count = argument_list();

    emit_bytes(OP_CALL_INIT, arg_count);
}

static void is_(bool can_assign) {
    XEN_UNUSED(can_assign);
    consume(TOKEN_IDENTIFIER, "expected type name after 'is'");
    u8 type_constant = identifier_constant(&parser.previous);
    emit_bytes(OP_IS_TYPE, type_constant);
}

static void as_(bool can_assign) {
    XEN_UNUSED(can_assign);
    consume(TOKEN_IDENTIFIER, "expected type name after 'as'");
    u8 type_constant = identifier_constant(&parser.previous);
    emit_bytes(OP_CAST, type_constant);
}

// ============================================================================
// Parse Rules
// ============================================================================

// clang-format off
xen_parse_rule rules[] = {
    [TOKEN_LEFT_PAREN]       = {grouping,   call,        PREC_CALL},
    [TOKEN_RIGHT_PAREN]      = {NULL,       NULL,        PREC_NONE},
    [TOKEN_LEFT_BRACE]       = {dictionary, NULL,        PREC_NONE},
    [TOKEN_RIGHT_BRACE]      = {NULL,       NULL,        PREC_NONE},
    [TOKEN_LEFT_BRACKET]     = {array_lit,  subscript,   PREC_CALL},
    [TOKEN_RIGHT_BRACKET]    = {NULL,       NULL,        PREC_NONE},
    [TOKEN_COMMA]            = {NULL,       NULL,        PREC_NONE},
    [TOKEN_DOT]              = {NULL,       dot,         PREC_CALL},
    [TOKEN_DOT_DOT]          = {NULL,       NULL,        PREC_NONE},
    [TOKEN_MINUS]            = {unary,      binary,      PREC_TERM},
    [TOKEN_PLUS]             = {NULL,       binary,      PREC_TERM},
    [TOKEN_SEMICOLON]        = {NULL,       NULL,        PREC_NONE},
    [TOKEN_FORWARD_SLASH]    = {NULL,       binary,      PREC_FACTOR},
    [TOKEN_ASTERISK]         = {NULL,       binary,      PREC_FACTOR},
    [TOKEN_PERCENT]          = {NULL,       binary,      PREC_FACTOR},
    [TOKEN_BANG]             = {unary,      NULL,        PREC_NONE},
    [TOKEN_BANG_EQUAL]       = {NULL,       binary,      PREC_EQUALITY},
    [TOKEN_EQUAL]            = {NULL,       NULL,        PREC_NONE},
    [TOKEN_EQUAL_EQUAL]      = {NULL,       binary,      PREC_EQUALITY},
    [TOKEN_GREATER]          = {NULL,       binary,      PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL]    = {NULL,       binary,      PREC_COMPARISON},
    [TOKEN_LESS]             = {NULL,       binary,      PREC_COMPARISON},
    [TOKEN_LESS_EQUAL]       = {NULL,       binary,      PREC_COMPARISON},
    [TOKEN_PLUS_PLUS]        = {prefix_inc, postfix_inc, PREC_POSTFIX},
    [TOKEN_MINUS_MINUS]      = {prefix_dec, postfix_dec, PREC_POSTFIX},
    [TOKEN_PLUS_EQUAL]       = {NULL,       NULL,        PREC_NONE},
    [TOKEN_MINUS_EQUAL]      = {NULL,       NULL,        PREC_NONE},
    [TOKEN_ASTERISK_EQUAL]   = {NULL,       NULL,        PREC_NONE},
    [TOKEN_SLASH_EQUAL]      = {NULL,       NULL,        PREC_NONE},
    [TOKEN_PERCENT_EQUAL]    = {NULL,       NULL,        PREC_NONE},
    [TOKEN_IDENTIFIER]       = {variable,   NULL,        PREC_NONE},
    [TOKEN_STRING]           = {string,     NULL,        PREC_NONE},
    [TOKEN_NUMBER]           = {number,     NULL,        PREC_NONE},
    [TOKEN_AND]              = {NULL,       and_,        PREC_AND},
    [TOKEN_CLASS]            = {NULL,       NULL,        PREC_NONE},
    [TOKEN_CONST]            = {NULL,       NULL,        PREC_NONE},
    [TOKEN_ELSE]             = {NULL,       NULL,        PREC_NONE},
    [TOKEN_FALSE]            = {literal,    NULL,        PREC_NONE},
    [TOKEN_FOR]              = {NULL,       NULL,        PREC_NONE},
    [TOKEN_FN]               = {fn_expr,    NULL,        PREC_NONE},
    [TOKEN_IF]               = {NULL,       NULL,        PREC_NONE},
    [TOKEN_IN]               = {NULL,       NULL,        PREC_NONE},
    [TOKEN_NULL]             = {literal,    NULL,        PREC_NONE},
    [TOKEN_OR]               = {NULL,       or_,         PREC_OR},
    [TOKEN_RETURN]           = {NULL,       NULL,        PREC_NONE},
    [TOKEN_TRUE]             = {literal,    NULL,        PREC_NONE},
    [TOKEN_VAR]              = {NULL,       NULL,        PREC_NONE},
    [TOKEN_WHILE]            = {NULL,       NULL,        PREC_NONE},
    [TOKEN_INCLUDE]          = {NULL,       NULL,        PREC_NONE},
    [TOKEN_THIS]             = {this_,      NULL,        PREC_NONE},
    [TOKEN_NEW]              = {new_expr,   NULL,        PREC_NONE},
    [TOKEN_AS]               = {NULL,       as_,         PREC_COMPARISON},
    [TOKEN_IS]               = {NULL,       is_,         PREC_COMPARISON},
    [TOKEN_ERROR]            = {NULL,       NULL,        PREC_NONE},
    [TOKEN_EOF]              = {NULL,       NULL,        PREC_NONE},
};
// clang-format on

static void expression() {
    parse_precedence(PREC_ASSIGNMENT);
}

static xen_parse_rule* get_rule(xen_token_type type) {
    return &rules[type];
}

static void parse_precedence(xen_op_prec prec) {
    advance();

    xen_parse_fn prefix_rule = get_rule(parser.previous.type)->prefix;
    if (!prefix_rule) {
        error("expected expression");
        return;
    }

    bool can_assign = prec <= PREC_ASSIGNMENT;
    prefix_rule(can_assign);

    while (prec <= get_rule(parser.current.type)->precedence) {
        advance();
        xen_parse_fn infix_rule = get_rule(parser.previous.type)->infix;
        infix_rule(can_assign);
    }

    if (can_assign && match_token(TOKEN_EQUAL)) {
        error("invalid assignment target");
    }
}

// ============================================================================
// Statements
// ============================================================================

static void synchronize() {
    parser.panic_mode = XEN_FALSE;

    while (!check(TOKEN_EOF)) {
        if (parser.previous.type == TOKEN_SEMICOLON)
            return;

        switch (parser.current.type) {
            case TOKEN_CLASS:
            case TOKEN_CONST:
            case TOKEN_FN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_RETURN:
            case TOKEN_INCLUDE:
                return;
            default:
                break;
        }

        advance();
    }
}

static void block() {
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        declaration();
    }

    consume(TOKEN_RIGHT_BRACE, "expected '}' after block");
}

static void expression_statement() {
    expression();
    consume(TOKEN_SEMICOLON, "expected ';' after expression");
    emit_byte(OP_POP);
}

static void return_statement() {
    if (current->type == TYPE_SCRIPT) {
        error("cannot return from top-level code");
    }

    if (match_token(TOKEN_SEMICOLON)) {
        emit_return();
    } else {
        expression();
        consume(TOKEN_SEMICOLON, "expected ';' after return value");
        emit_byte(OP_RETURN);
    }
}

static void if_statement() {
    // parse condition
    consume(TOKEN_LEFT_PAREN, "expected '(' after 'if'");
    expression();
    consume(TOKEN_RIGHT_PAREN, "expected ')' after condition");

    // emit conditional jump (will skip 'then' branch if false)
    i32 then_jump = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP);  // pop condition from stack

    // compile 'then' branch
    statement();

    // emit unconditional jump (will skip 'else' branch)
    i32 else_jump = emit_jump(OP_JUMP);

    // patch the conditional jump to land here
    patch_jump(then_jump);
    emit_byte(OP_POP);  // pop condition (for false path)

    // compile 'else' branch (if present)
    if (match_token(TOKEN_ELSE)) {
        statement();
    }

    patch_jump(else_jump);
}

static void while_statement() {
    i32 loop_start = current_chunk()->count;  // remember where to jump back

    // parse condition
    consume(TOKEN_LEFT_PAREN, "expected '(' after 'while'");
    expression();
    consume(TOKEN_RIGHT_PAREN, "expected ')' after condition");

    // exit jump (when condition is false)
    i32 exit_jump = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP);  // pop condition

    // compile body
    statement();

    // jump back to condition
    emit_loop(loop_start);

    // patch exit jump
    patch_jump(exit_jump);
    emit_byte(OP_POP);
}

static void var_declaration();

/*
 * for-in statement: for (var i in start..end) { body }
 * Desugars to:
 *   {
 *     var i = start;
 *     var __end = end;  // hidden variable
 *     while (i < __end) {
 *       body;
 *       i = i + 1;
 *     }
 *   }
 */
static void for_in_statement() {
    begin_scope();

    // parse: for (var i in start..end)
    consume(TOKEN_LEFT_PAREN, "expected '(' after 'for'");
    consume(TOKEN_VAR, "expected 'var' in for-in loop");
    consume(TOKEN_IDENTIFIER, "expected loop variable name");

    // declare the loop variable
    xen_token loop_var = parser.previous;
    add_local(loop_var, XEN_FALSE);

    consume(TOKEN_IN, "expected 'in' after loop variable");

    // compile start expression and initialize loop variable
    expression();
    mark_initialized();  // loop var is now slot (local_count - 1)
    u8 loop_var_slot = (u8)(current->local_count - 1);

    consume(TOKEN_DOT_DOT, "expected '..' in range");

    // compile end expression into a hidden local
    expression();

    // create hidden __end variable
    xen_token end_token;
    end_token.start  = "__end";
    end_token.length = 5;
    end_token.line   = parser.previous.line;
    end_token.type   = TOKEN_IDENTIFIER;
    add_local(end_token, XEN_FALSE);
    mark_initialized();
    u8 end_var_slot = (u8)(current->local_count - 1);

    consume(TOKEN_RIGHT_PAREN, "expected ')' after range");

    // loop_start: where we jump back to for each iteration
    i32 loop_start = current_chunk()->count;

    // condition: i < __end
    emit_bytes(OP_GET_LOCAL, loop_var_slot);
    emit_bytes(OP_GET_LOCAL, end_var_slot);
    emit_byte(OP_LESS);

    // exit jump (when condition is false)
    i32 exit_jump = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP);  // pop condition

    // compile body
    statement();

    // increment: i = i + 1
    emit_bytes(OP_GET_LOCAL, loop_var_slot);
    emit_constant(NUMBER_VAL(1));
    emit_byte(OP_ADD);
    emit_bytes(OP_SET_LOCAL, loop_var_slot);
    emit_byte(OP_POP);  // pop the result of the assignment

    // jump back to condition
    emit_loop(loop_start);

    // patch exit jump
    patch_jump(exit_jump);
    emit_byte(OP_POP);  // pop condition (false case)

    end_scope();
}

/*
 * C-style for statement: for (init; cond; incr) { body }
 */
static void for_c_style_statement() {
    begin_scope();  // for loop variable
    consume(TOKEN_LEFT_PAREN, "expected '(' after 'for'");

    // initializer clause
    if (match_token(TOKEN_SEMICOLON)) {
        // no initializer
    } else if (match_token(TOKEN_VAR)) {
        var_declaration();
    } else {
        expression_statement();
    }

    i32 loop_start = current_chunk()->count;

    // condition clause
    i32 exit_jump = -1;
    if (!match_token(TOKEN_SEMICOLON)) {
        expression();
        consume(TOKEN_SEMICOLON, "expected ';' after loop condition");
        exit_jump = emit_jump(OP_JUMP_IF_FALSE);
        emit_byte(OP_POP);
    }

    // increment clause (compiled but executed after body)
    if (!match_token(TOKEN_RIGHT_PAREN)) {
        i32 body_jump       = emit_jump(OP_JUMP);
        i32 increment_start = current_chunk()->count;
        expression();
        emit_byte(OP_POP);
        consume(TOKEN_RIGHT_PAREN, "expected ')' after for clauses");
        emit_loop(loop_start);
        loop_start = increment_start;
        patch_jump(body_jump);
    }

    // body
    statement();
    emit_loop(loop_start);

    if (exit_jump != -1) {
        patch_jump(exit_jump);
        emit_byte(OP_POP);
    }

    end_scope();
}

/*
 * Dispatch for-statement based on syntax:
 *   for (var i in 0..10) { ... }   -> for-in (range-based)
 *   for (init; cond; incr) { ... } -> C-style
 */
static void for_statement() {
    // peek ahead to determine which kind of for-loop
    // if we see "var IDENT in" then it's for-in, otherwise C-style

    if (check(TOKEN_LEFT_PAREN)) {
        // We need to look further ahead. Let's consume '(' and check
        advance();  // consume '('

        if (check(TOKEN_VAR)) {
            // Could be either "var i = ..." (C-style) or "var i in ..." (for-in)
            advance();  // consume 'var'

            if (check(TOKEN_IDENTIFIER)) {
                advance();  // consume identifier

                if (check(TOKEN_IN)) {
                    // It's a for-in loop: for (var i in start..end) OR for (var x in array)
                    // We've already consumed '(', 'var', and the identifier
                    xen_token loop_var = parser.previous;  // the identifier

                    begin_scope();

                    consume(TOKEN_IN, "expected 'in' after loop variable");

                    // Parse the expression after 'in'
                    expression();

                    // Now check: is this a range (has ..) or an array iteration?
                    if (match_token(TOKEN_DOT_DOT)) {
                        /* ============================================
                         * RANGE-BASED FOR LOOP: for (var i in start..end)
                         * ============================================
                         * Desugars to:
                         *   var i = start;
                         *   var __end = end;
                         *   while (i < __end) { body; i = i + 1; }
                         */

                        // The start value is already on the stack from expression() above
                        // Declare loop variable and initialize with start value
                        add_local(loop_var, XEN_FALSE);
                        mark_initialized();
                        u8 loop_var_slot = (u8)(current->local_count - 1);

                        // Compile end expression into a hidden local
                        expression();

                        // Create hidden __end variable
                        xen_token end_token;
                        end_token.start  = "__end";
                        end_token.length = 5;
                        end_token.line   = parser.previous.line;
                        end_token.type   = TOKEN_IDENTIFIER;
                        add_local(end_token, XEN_FALSE);
                        mark_initialized();
                        u8 end_var_slot = (u8)(current->local_count - 1);

                        consume(TOKEN_RIGHT_PAREN, "expected ')' after range");

                        // loop_start
                        i32 loop_start = current_chunk()->count;

                        // condition: i < __end
                        emit_bytes(OP_GET_LOCAL, loop_var_slot);
                        emit_bytes(OP_GET_LOCAL, end_var_slot);
                        emit_byte(OP_LESS);

                        i32 exit_jump = emit_jump(OP_JUMP_IF_FALSE);
                        emit_byte(OP_POP);

                        // body
                        statement();

                        // increment: i = i + 1
                        emit_bytes(OP_GET_LOCAL, loop_var_slot);
                        emit_constant(NUMBER_VAL(1));
                        emit_byte(OP_ADD);
                        emit_bytes(OP_SET_LOCAL, loop_var_slot);
                        emit_byte(OP_POP);

                        emit_loop(loop_start);

                        patch_jump(exit_jump);
                        emit_byte(OP_POP);

                        end_scope();
                        return;
                    } else {
                        /* ============================================
                         * ARRAY-BASED FOR LOOP: for (var x in array)
                         * ============================================
                         * Desugars to:
                         *   var __arr = array;
                         *   var __len = array.count;  (using OP_ARRAY_LEN)
                         *   var __i = 0;
                         *   while (__i < __len) {
                         *       var x = __arr[__i];
                         *       body;
                         *       __i = __i + 1;
                         *   }
                         */

                        // The array value is on the stack from expression() above
                        // Store it in hidden __arr variable
                        xen_token arr_token;
                        arr_token.start  = "__arr";
                        arr_token.length = 5;
                        arr_token.line   = parser.previous.line;
                        arr_token.type   = TOKEN_IDENTIFIER;
                        add_local(arr_token, XEN_FALSE);
                        mark_initialized();
                        u8 arr_slot = (u8)(current->local_count - 1);

                        // Get array length and store in __len
                        emit_bytes(OP_GET_LOCAL, arr_slot);
                        emit_byte(OP_ARRAY_LEN);

                        xen_token len_token;
                        len_token.start  = "__len";
                        len_token.length = 5;
                        len_token.line   = parser.previous.line;
                        len_token.type   = TOKEN_IDENTIFIER;
                        add_local(len_token, XEN_FALSE);
                        mark_initialized();
                        u8 len_slot = (u8)(current->local_count - 1);

                        // Initialize index __i = 0
                        emit_constant(NUMBER_VAL(0));

                        xen_token idx_token;
                        idx_token.start  = "__i";
                        idx_token.length = 3;
                        idx_token.line   = parser.previous.line;
                        idx_token.type   = TOKEN_IDENTIFIER;
                        add_local(idx_token, XEN_FALSE);
                        mark_initialized();
                        u8 idx_slot = (u8)(current->local_count - 1);

                        consume(TOKEN_RIGHT_PAREN, "expected ')' after array expression");

                        // loop_start
                        i32 loop_start = current_chunk()->count;

                        // condition: __i < __len
                        emit_bytes(OP_GET_LOCAL, idx_slot);
                        emit_bytes(OP_GET_LOCAL, len_slot);
                        emit_byte(OP_LESS);

                        i32 exit_jump = emit_jump(OP_JUMP_IF_FALSE);
                        emit_byte(OP_POP);

                        // Declare loop variable and set to __arr[__i]
                        emit_bytes(OP_GET_LOCAL, arr_slot);
                        emit_bytes(OP_GET_LOCAL, idx_slot);
                        emit_byte(OP_INDEX_GET);

                        add_local(loop_var, XEN_FALSE);
                        mark_initialized();

                        // body
                        statement();

                        // Pop the loop variable (it goes out of scope each iteration)
                        emit_byte(OP_POP);
                        current->local_count--;

                        // increment: __i = __i + 1
                        emit_bytes(OP_GET_LOCAL, idx_slot);
                        emit_constant(NUMBER_VAL(1));
                        emit_byte(OP_ADD);
                        emit_bytes(OP_SET_LOCAL, idx_slot);
                        emit_byte(OP_POP);

                        emit_loop(loop_start);

                        patch_jump(exit_jump);
                        emit_byte(OP_POP);

                        end_scope();
                        return;
                    }
                } else {
                    // It's C-style: for (var i = ...; ...; ...)
                    // We've consumed '(', 'var', identifier
                    // The identifier is a variable declaration, continue from here

                    begin_scope();

                    // We already have the identifier in parser.previous
                    // Declare it as a local
                    xen_token var_name = parser.previous;
                    add_local(var_name, XEN_FALSE);

                    if (match_token(TOKEN_EQUAL)) {
                        expression();
                    } else {
                        emit_byte(OP_NULL);
                    }
                    consume(TOKEN_SEMICOLON, "expected ';' after variable declaration");
                    mark_initialized();

                    // Now continue with normal C-style for loop
                    i32 loop_start = current_chunk()->count;

                    i32 exit_jump = -1;
                    if (!match_token(TOKEN_SEMICOLON)) {
                        expression();
                        consume(TOKEN_SEMICOLON, "expected ';' after loop condition");
                        exit_jump = emit_jump(OP_JUMP_IF_FALSE);
                        emit_byte(OP_POP);
                    }

                    if (!match_token(TOKEN_RIGHT_PAREN)) {
                        i32 body_jump       = emit_jump(OP_JUMP);
                        i32 increment_start = current_chunk()->count;
                        expression();
                        emit_byte(OP_POP);
                        consume(TOKEN_RIGHT_PAREN, "expected ')' after for clauses");
                        emit_loop(loop_start);
                        loop_start = increment_start;
                        patch_jump(body_jump);
                    }

                    statement();
                    emit_loop(loop_start);

                    if (exit_jump != -1) {
                        patch_jump(exit_jump);
                        emit_byte(OP_POP);
                    }

                    end_scope();
                    return;
                }
            }
        }

        /* If we get here, it's a C-style for loop without 'var',
         * or we've already consumed '(' */
        // Handle: for (; cond; incr) or for (expr; cond; incr)

        begin_scope();

        // initializer - we already consumed '('
        if (match_token(TOKEN_SEMICOLON)) {
            // no initializer
        } else {
            // expression statement as initializer
            expression();
            consume(TOKEN_SEMICOLON, "expected ';' after loop initializer");
            emit_byte(OP_POP);
        }

        i32 loop_start = current_chunk()->count;

        i32 exit_jump = -1;
        if (!match_token(TOKEN_SEMICOLON)) {
            expression();
            consume(TOKEN_SEMICOLON, "expected ';' after loop condition");
            exit_jump = emit_jump(OP_JUMP_IF_FALSE);
            emit_byte(OP_POP);
        }

        if (!match_token(TOKEN_RIGHT_PAREN)) {
            i32 body_jump       = emit_jump(OP_JUMP);
            i32 increment_start = current_chunk()->count;
            expression();
            emit_byte(OP_POP);
            consume(TOKEN_RIGHT_PAREN, "expected ')' after for clauses");
            emit_loop(loop_start);
            loop_start = increment_start;
            patch_jump(body_jump);
        }

        statement();
        emit_loop(loop_start);

        if (exit_jump != -1) {
            patch_jump(exit_jump);
            emit_byte(OP_POP);
        }

        end_scope();
    } else {
        error("expected '(' after 'for'");
    }
}

static void statement() {
    if (match_token(TOKEN_IF)) {
        if_statement();
    } else if (match_token(TOKEN_WHILE)) {
        while_statement();
    } else if (match_token(TOKEN_FOR)) {
        for_statement();
    } else if (match_token(TOKEN_RETURN)) {
        return_statement();
    } else if (match_token(TOKEN_LEFT_BRACE)) {
        begin_scope();
        block();
        end_scope();
    } else {
        expression_statement();
    }
}

// ============================================================================
// Declarations
// ============================================================================

static void var_declaration_impl(bool is_const) {
    u8 global          = parse_variable("expected variable name");
    xen_token var_name = parser.previous;  // Save the variable name for const tracking

    if (is_const) {
        if (!match_token(TOKEN_EQUAL)) {
            error("constant variables must be initialized");
            // Still need to emit something and consume semicolon
            emit_byte(OP_NULL);
            consume(TOKEN_SEMICOLON, "expected ';' after variable declaration");
            define_variable(global);
            return;
        }
        expression();
    } else {
        if (match_token(TOKEN_EQUAL)) {
            expression();
        } else {
            emit_byte(OP_NULL);
        }
    }

    consume(TOKEN_SEMICOLON, "expected ';' after variable declaration");

    // Track constness
    if (current->scope_depth > 0) {
        // For locals, update the is_const flag (add_local was called in declare_variable)
        current->locals[current->local_count - 1].is_const = is_const;
    } else {
        // For globals, record in our const table
        if (is_const) {
            mark_global_const(&var_name);
        }
    }

    define_variable(global);
}

static void var_declaration() {
    var_declaration_impl(XEN_FALSE);
}

static void const_var_declaration() {
    // We've already consumed 'const', now expect 'var'
    consume(TOKEN_VAR, "expected 'var' after 'const'");
    var_declaration_impl(XEN_TRUE);
}

static void property_declaration(bool is_private) {
    consume(TOKEN_IDENTIFIER, "expect property name");
    u8 name_constant = identifier_constant(&parser.previous);

    if (match_token(TOKEN_EQUAL)) {
        expression();
    } else {
        emit_byte(OP_NULL);
    }

    consume(TOKEN_SEMICOLON, "expect ';' after property declaration");

    // Stack: [class, default_value]
    emit_bytes(OP_PROPERTY, name_constant);
    emit_byte(is_private ? 1 : 0);
}

static void method(xen_function_type type) {
    xen_compiler compiler;
    init_compiler(&compiler, type);
    begin_scope();

    consume(TOKEN_LEFT_PAREN, "expect '(' after method name");

    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            current->function->arity++;
            if (current->function->arity > 255) {
                error_at_current("cannot have more than 255 parameters");
            }
            u8 constant = parse_variable("expect parameter name");
            define_variable(constant);
        } while (match_token(TOKEN_COMMA));
    }

    consume(TOKEN_RIGHT_PAREN, "expected ')' after parameters");

    if (match_token(TOKEN_ARROW)) {
        // Single expression body, implicit return
        expression();
        emit_byte(OP_RETURN);
    } else {
        // Traditional block body
        consume(TOKEN_LEFT_BRACE, "expected '{' before function body");
        block();
    }

    xen_obj_func* fn = end_compiler();
    emit_bytes(OP_CONSTANT, make_constant(OBJ_VAL(fn)));
}

static void method_declaration(bool is_private) {
    consume(TOKEN_IDENTIFIER, "expect method name");
    u8 name_constant = identifier_constant(&parser.previous);

    xen_function_type type = TYPE_METHOD;
    method(type);

    emit_bytes(OP_METHOD, name_constant);
    emit_byte(is_private ? 1 : 0);
}

static void init_declaration() {
    xen_function_type type = TYPE_INITIALIZER;
    method(type);
    emit_byte(OP_INITIALIZER);
}

static void class_declaration() {
    consume(TOKEN_IDENTIFIER, "expect class name");
    xen_token class_name = parser.previous;
    u8 name_constant     = identifier_constant(&class_name);

    declare_variable();

    emit_bytes(OP_CLASS, name_constant);
    define_variable(name_constant);

    class_compiler comp;
    comp.enclosing = current_class;
    comp.name      = class_name;
    current_class  = &comp;

    named_variable(class_name, XEN_FALSE);

    consume(TOKEN_LEFT_BRACE, "expect '{' before class body");

    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        bool is_private = XEN_FALSE;

        if (match_token(TOKEN_PRIVATE)) {
            is_private = XEN_TRUE;
        }

        if (match_token(TOKEN_FN)) {
            method_declaration(is_private);
        } else if (match_token(TOKEN_INIT)) {
            init_declaration();
        } else if (check(TOKEN_IDENTIFIER)) {
            property_declaration(is_private);
        } else {
            error("expect property or method declaration");
            advance();
        }
    }

    consume(TOKEN_RIGHT_BRACE, "expect '}' after class body");
    consume(TOKEN_SEMICOLON, "expect ';' after class definition");

    emit_byte(OP_POP);  // Pop the class
    current_class = current_class->enclosing;
}

static void function(xen_function_type type) {
    xen_compiler compiler;
    init_compiler(&compiler, type);
    begin_scope();

    consume(TOKEN_LEFT_PAREN, "expected '(' after function name");

    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            current->function->arity++;
            if (current->function->arity > 255) {
                error_at_current("cannot have more than 255 parameters");
            }
            u8 constant = parse_variable("expected parameter name");
            define_variable(constant);
        } while (match_token(TOKEN_COMMA));
    }

    consume(TOKEN_RIGHT_PAREN, "expected ')' after parameters");

    if (match_token(TOKEN_ARROW)) {
        // Single expression body, implicit return
        expression();
        emit_byte(OP_RETURN);
    } else {
        // Traditional block body
        consume(TOKEN_LEFT_BRACE, "expected '{' before function body");
        block();
    }

    xen_obj_func* fn = end_compiler();
    emit_bytes(OP_CONSTANT, make_constant(OBJ_VAL(fn)));
}

static void fn_declaration() {
    u8 global = parse_variable("expected function name");
    mark_initialized();
    function(TYPE_FUNCTION);
    define_variable(global);
}

static void include_declaration() {
    consume(TOKEN_IDENTIFIER, "expected namespace name after 'include'");
    xen_token name = parser.previous;

    if (!is_valid_namespace(name.start, name.length)) {
        error("unknown namespace");
        return;
    }

    consume(TOKEN_SEMICOLON, "expected ';' after include statement");
    u8 name_constant = identifier_constant(&name);
    emit_bytes(OP_INCLUDE, name_constant);
}

static void declaration() {
    if (match_token(TOKEN_FN)) {
        fn_declaration();
    } else if (match_token(TOKEN_CLASS)) {
        class_declaration();
    } else if (match_token(TOKEN_CONST)) {
        const_var_declaration();
    } else if (match_token(TOKEN_VAR)) {
        var_declaration();
    } else if (match_token(TOKEN_INCLUDE)) {
        include_declaration();
    } else {
        statement();
    }

    if (parser.panic_mode) {
        synchronize();
    }
}

// ============================================================================
// Entry Point
// ============================================================================

xen_obj_func* xen_compile(const char* source) {
    xen_scanner_init(source);

    xen_compiler compiler;
    init_compiler(&compiler, TYPE_SCRIPT);

    parser.had_error         = XEN_FALSE;
    parser.panic_mode        = XEN_FALSE;
    parser.last_was_variable = XEN_FALSE;

    advance();

    while (!check(TOKEN_EOF)) {
        declaration();
    }

    xen_obj_func* fn = end_compiler();

    const char* name = "main";
    fn->name         = xen_obj_str_copy(name, strlen(name));
    return parser.had_error ? NULL : fn;
}

static char* load_file_source(const char* path) {
    return xen_read_file(path);
}

static void compile_namespaced_declarations(const char* namespace_name) {}

static void compile_file_include(const char* path, const char* namespace_name) {
    // Check for circular includes
    if (is_already_included(path)) {
        return;  // Silently skip - already included
    }
    mark_as_included(path);

    // Load the source file
    char* source = load_file_source(path);
    if (!source) {
        error("could not load include file");
        return;
    }

    // Save current compilation context
    xen_token_scanner saved_scanner = xen_scanner_save();
    xen_token saved_current         = parser.current;
    xen_token saved_previous        = parser.previous;
    char saved_path[MAX_PATH_LENGTH];
    strncpy(saved_path, current_file_path, MAX_PATH_LENGTH);

    // Update current file path for nested includes
    strncpy(current_file_path, path, MAX_PATH_LENGTH);

    xen_scanner_init(source);
    advance();

    if (namespace_name != NULL) {
        // Create namespace
        compile_namespaced_declarations(namespace_name);
    } else {
        // Compile directly into global scope
        while (!check(TOKEN_EOF)) {
            declaration();
        }
    }

    // Restore compilation context
    xen_scanner_restore(saved_scanner);
    parser.current  = saved_current;
    parser.previous = saved_previous;
    strncpy(current_file_path, saved_path, MAX_PATH_LENGTH);

    free(source);
}
