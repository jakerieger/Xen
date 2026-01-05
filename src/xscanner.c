#include "xscanner.h"
#include "xalloc.h"
#include "xcommon.h"

typedef struct {
    const char* start;
    const char* current;
    i32 line;
} xen_token_scanner;

xen_token_scanner scanner;

void xen_scanner_init(const char* source) {
    scanner.start   = source;
    scanner.current = source;
    scanner.line    = 1;
}

static bool is_at_end() {
    return *scanner.current == '\0';
}

static xen_token make_token(const xen_token_type type) {
    xen_token token;
    token.type   = type;
    token.start  = scanner.start;
    token.length = (i32)(scanner.current - scanner.start);
    token.line   = scanner.line;
    return token;
}

static xen_token make_error_token(const char* msg) {
    xen_token token;
    token.type   = TOKEN_ERROR;
    token.start  = msg;
    token.length = (i32)strlen(msg);
    token.line   = scanner.line;
    return token;
}

static char advance() {
    scanner.current++;
    return scanner.current[-1];
}

static bool match(const char expected) {
    if (is_at_end())
        return XEN_TRUE;
    if (*scanner.current != expected)
        return XEN_FALSE;
    scanner.current++;
    return XEN_TRUE;
}

static char peek() {
    return *scanner.current;
}

static char peek_next() {
    if (is_at_end()) {
        return '\0';
    }
    return scanner.current[1];
}

static void skip_whitespace() {
    for (;;) {
        const char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            case '\n':
                scanner.line++;
                advance();
                break;
            case '/':
                if (peek_next() == '/') {
                    while (peek() != '\n' && !is_at_end())
                        advance();
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

/// @brief [a-z] || [A-Z] || [_]
static bool is_alpha(const char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

/// @brief [0-9]
static bool is_digit(const char c) {
    return c >= '0' && c <= '9';
}

static xen_token_type check_keyword(const i32 start, const i32 length, const char* rest, const xen_token_type type) {
    if (scanner.current - scanner.start == start + length && memcmp(scanner.start + start, rest, length) == 0) {
        return type;
    }
    return TOKEN_IDENTIFIER;
}

static xen_token_type identifier_type() {
    switch (scanner.start[0]) {
        case 'a':
            return check_keyword(1, 2, "nd", TOKEN_AND);
        case 'c':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'l':
                        return check_keyword(2, 3, "ass", TOKEN_CLASS);
                    case 'o':
                        return check_keyword(2, 3, "nst", TOKEN_CONST);
                }
            }
            break;
        case 'e':
            return check_keyword(1, 3, "lse", TOKEN_ELSE);
        case 'f':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'a':
                        return check_keyword(2, 3, "lse", TOKEN_FALSE);
                    case 'o':
                        return check_keyword(2, 1, "r", TOKEN_FOR);
                    case 'n':
                        return check_keyword(2, 0, "", TOKEN_FN);
                }
            }
            break;
        case 'i':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'f':
                        return check_keyword(2, 0, "", TOKEN_IF);
                    case 'n':
                        if (scanner.current - scanner.start == 2) {
                            return TOKEN_IN;
                        }
                        if (scanner.current - scanner.start == 4) {
                            return check_keyword(2, 2, "it", TOKEN_INIT);
                        }
                        return check_keyword(2, 5, "clude", TOKEN_INCLUDE);
                }
            }
            break;
        case 'n':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'u':
                        return check_keyword(2, 2, "ll", TOKEN_NULL);
                    case 'e':
                        return check_keyword(2, 1, "w", TOKEN_NEW);
                }
            }
            break;
        case 'o':
            return check_keyword(1, 1, "r", TOKEN_OR);
        case 'p':
            return check_keyword(1, 6, "rivate", TOKEN_PRIVATE);
        case 'r':
            return check_keyword(1, 5, "eturn", TOKEN_RETURN);
        case 't':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'h':
                        return check_keyword(2, 2, "is", TOKEN_THIS);
                    case 'r':
                        return check_keyword(2, 2, "ue", TOKEN_TRUE);
                }
            }
            break;
        case 'v':
            return check_keyword(1, 2, "ar", TOKEN_VAR);
        case 'w':
            return check_keyword(1, 4, "hile", TOKEN_WHILE);
    }

    return TOKEN_IDENTIFIER;
}

static xen_token make_identifier() {
    while (is_alpha(peek()) || is_digit(peek())) {
        advance();
    }
    return make_token(identifier_type());
}

static xen_token make_string() {
    while (peek() != '"' && !is_at_end()) {
        if (peek() == '\n')
            scanner.line++;
        advance();
    }

    if (is_at_end())
        return make_error_token("unterminated string");

    advance();
    return make_token(TOKEN_STRING);
}

static xen_token make_number() {
    while (is_digit(peek()))
        advance();

    if (peek() == '.' && is_digit(peek_next())) {
        advance();

        while (is_digit(peek()))
            advance();
    }

    return make_token(TOKEN_NUMBER);
}

xen_token xen_scanner_emit() {
    skip_whitespace();

    scanner.start = scanner.current;
    if (is_at_end()) {
        return make_token(TOKEN_EOF);
    }

    const char c = advance();
    if (is_alpha(c))
        return make_identifier();
    if (is_digit(c))
        return make_number();

    switch (c) {
        case '(':
            return make_token(TOKEN_LEFT_PAREN);
        case ')':
            return make_token(TOKEN_RIGHT_PAREN);
        case '{':
            return make_token(TOKEN_LEFT_BRACE);
        case '}':
            return make_token(TOKEN_RIGHT_BRACE);
        case '[':
            return make_token(TOKEN_LEFT_BRACKET);
        case ']':
            return make_token(TOKEN_RIGHT_BRACKET);
        case ';':
            return make_token(TOKEN_SEMICOLON);
        case ',':
            return make_token(TOKEN_COMMA);
        case ':':
            return make_token(TOKEN_COLON);
        case '.':
            return make_token(match('.') ? TOKEN_DOT_DOT : TOKEN_DOT);
        case '-': {
            if (match('-')) {
                return make_token(TOKEN_MINUS_MINUS);
            } else if (match('=')) {
                return make_token(TOKEN_MINUS_EQUAL);
            }
            return make_token(TOKEN_MINUS);
        }
        case '+': {
            if (match('+')) {
                return make_token(TOKEN_PLUS_PLUS);
            } else if (match('=')) {
                return make_token(TOKEN_PLUS_EQUAL);
            }
            return make_token(TOKEN_PLUS);
        }
        case '/':
            return make_token(match('=') ? TOKEN_SLASH_EQUAL : TOKEN_FORWARD_SLASH);
        case '*':
            return make_token(match('=') ? TOKEN_ASTERISK_EQUAL : TOKEN_ASTERISK);
        case '%':
            return make_token(match('=') ? TOKEN_PERCENT_EQUAL : TOKEN_PERCENT);
        case '!':
            return make_token(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '=': {
            if (match('=')) {
                return make_token(TOKEN_EQUAL_EQUAL);
            } else if (match('>')) {
                return make_token(TOKEN_ARROW);
            }
            return make_token(TOKEN_EQUAL);
        }
        case '<':
            return make_token(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        case '>':
            return make_token(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
        case '"':
            return make_string();
        default:
            break;
    }

    return make_error_token("unexpected character");
}

void xen_token_print(xen_token* token) {
    const char* type_str = xen_token_type_to_str(token->type);
    printf("Token: [%s] ", type_str);

    if (token->type == TOKEN_IDENTIFIER || token->type == TOKEN_STRING || token->type == TOKEN_NUMBER) {
        printf("'%.*s'", token->length, token->start);
    }

    printf("\n");
}

const char* xen_token_type_to_str(xen_token_type type) {
    switch (type) {
        case TOKEN_PLUS:
            return "+";
        case TOKEN_MINUS:
            return "-";
        case TOKEN_ASTERISK:
            return "*";
        case TOKEN_DOUBLE_ASTERISK:
            return "**";
        case TOKEN_FORWARD_SLASH:
            return "/";
        case TOKEN_DOUBLE_FORWARD_SLASH:
            return "//";
        case TOKEN_COMMA:
            return ",";
        case TOKEN_EQUAL:
            return "=";
        case TOKEN_IDENTIFIER:
            return "identifier";
        case TOKEN_LEFT_PAREN:
            return "(";
        case TOKEN_RIGHT_PAREN:
            return ")";
        case TOKEN_NUMBER:
            return "number";
        case TOKEN_EOF:
            return "EOF";
        case TOKEN_PERCENT:
            return "%";
        case TOKEN_LEFT_BRACE:
            return "{";
        case TOKEN_RIGHT_BRACE:
            return "}";
        case TOKEN_DOT:
            return ".";
        case TOKEN_SEMICOLON:
            return ";";
        case TOKEN_BANG:
            return "!";
        case TOKEN_BANG_EQUAL:
            return "!=";
        case TOKEN_EQUAL_EQUAL:
            return "==";
        case TOKEN_GREATER:
            return ">";
        case TOKEN_GREATER_EQUAL:
            return ">=";
        case TOKEN_LESS:
            return "<";
        case TOKEN_LESS_EQUAL:
            return "<=";
        case TOKEN_STRING:
            return "string";
        case TOKEN_AND:
            return "&";
        case TOKEN_CLASS:
            return "class";
        case TOKEN_ELSE:
            return "else";
        case TOKEN_FALSE:
            return "false";
        case TOKEN_FOR:
            return "for";
        case TOKEN_FN:
            return "fn";
        case TOKEN_IF:
            return "if";
        case TOKEN_NULL:
            return "null";
        case TOKEN_OR:
            return "or";
        case TOKEN_RETURN:
            return "return";
        case TOKEN_THIS:
            return "this";
        case TOKEN_TRUE:
            return "true";
        case TOKEN_VAR:
            return "var";
        case TOKEN_WHILE:
            return "while";
        case TOKEN_ERROR:
            return "error";
        case TOKEN_PLUS_EQUAL:
            return "+=";
        case TOKEN_MINUS_EQUAL:
            return "-=";
        case TOKEN_ASTERISK_EQUAL:
            return "*=";
        case TOKEN_SLASH_EQUAL:
            return "/=";
        case TOKEN_PLUS_PLUS:
            return "++";
        case TOKEN_MINUS_MINUS:
            return "--";
        case TOKEN_DOT_DOT:
            return "..";
        case TOKEN_IN:
            return "in";
        case TOKEN_PERCENT_EQUAL:
            return "%=";
        case TOKEN_INCLUDE:
            return "include";
        case TOKEN_LEFT_BRACKET:
            return "[";
        case TOKEN_RIGHT_BRACKET:
            return "]";
        case TOKEN_CONST:
            return "const";
        case TOKEN_COLON:
            return ":";
        case TOKEN_ARROW:
            return "=>";
        case TOKEN_INIT:
            return "init";
        case TOKEN_NEW:
            return "new";
        case TOKEN_PRIVATE:
            return "private";
    }
    return "";
}
