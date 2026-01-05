#ifndef X_SCANNER_H
#define X_SCANNER_H

#include "xcommon.h"
#include "xalloc.h"

typedef enum {
    // Single-character tokens
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE,
    TOKEN_RIGHT_BRACE,
    TOKEN_LEFT_BRACKET,
    TOKEN_RIGHT_BRACKET,
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_MINUS,
    TOKEN_PLUS,
    TOKEN_SEMICOLON,
    TOKEN_FORWARD_SLASH,
    TOKEN_DOUBLE_FORWARD_SLASH,
    TOKEN_ASTERISK,
    TOKEN_DOUBLE_ASTERISK,
    TOKEN_PERCENT,
    TOKEN_COLON,
    // One or two character tokens
    TOKEN_BANG,
    TOKEN_BANG_EQUAL,
    TOKEN_EQUAL,
    TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER,
    TOKEN_GREATER_EQUAL,
    TOKEN_LESS,
    TOKEN_LESS_EQUAL,
    TOKEN_PLUS_EQUAL,      // +=
    TOKEN_MINUS_EQUAL,     // -=
    TOKEN_ASTERISK_EQUAL,  // *=
    TOKEN_SLASH_EQUAL,     // /=
    TOKEN_PLUS_PLUS,       // ++
    TOKEN_MINUS_MINUS,     // --
    TOKEN_DOT_DOT,         // ..
    TOKEN_PERCENT_EQUAL,   // %=
    TOKEN_ARROW,           // =>
    // Literals
    TOKEN_IDENTIFIER,
    TOKEN_STRING,
    TOKEN_NUMBER,
    // Keywords
    TOKEN_AND,
    TOKEN_CLASS,
    TOKEN_CONST,
    TOKEN_ELSE,
    TOKEN_FALSE,
    TOKEN_FOR,
    TOKEN_FN,
    TOKEN_IF,
    TOKEN_NULL,
    TOKEN_OR,
    TOKEN_RETURN,
    TOKEN_THIS,
    TOKEN_TRUE,
    TOKEN_VAR,
    TOKEN_WHILE,
    TOKEN_IN,
    TOKEN_INCLUDE,
    TOKEN_INIT,
    TOKEN_NEW,
    TOKEN_PRIVATE,
    // Signal tokens
    TOKEN_ERROR,
    TOKEN_EOF,
} xen_token_type;

typedef struct {
    xen_token_type type;
    const char* start;
    i32 length;
    i32 line;
} xen_token;

void xen_token_print(xen_token* token);
const char* xen_token_type_to_str(xen_token_type type);

void xen_scanner_init(const char* source);
xen_token xen_scanner_emit();

#endif
