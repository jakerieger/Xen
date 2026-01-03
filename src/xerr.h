#ifndef X_ERR_H
#define X_ERR_H

#include "xterminal.h"

#include <stdlib.h>
#include <stdnoreturn.h>

/*
 * EXIT CODES FOR COMMON ERRORS
 */
#define XEN_ERR_ALLOCATION_FAILED 101
#define XEN_ERR_OVER_CAPACITY 102
#define XEN_ERR_EXEC_COMPILE 103
#define XEN_ERR_EXEC_RUNTIME 104
#define XEN_ERR_OPEN_FILE 105
#define XEN_ERR_EXPECTED_EXPRESSION 106
#define XEN_ERR_INVALID_ARGS 107

inline static char* xen_exit_code_to_str(int code) {
    switch (code) {
        case XEN_ERR_ALLOCATION_FAILED:
            return "XEN_ERR_ALLOCATION_FAILED";
        case XEN_ERR_OVER_CAPACITY:
            return "XEN_ERR_OVER_CAPACITY";
        case XEN_ERR_EXEC_COMPILE:
            return "XEN_ERR_EXEC_COMPILE";
        case XEN_ERR_EXEC_RUNTIME:
            return "XEN_ERR_EXEC_RUNTIME";
        case XEN_ERR_OPEN_FILE:
            return "XEN_ERR_OPEN_FILE";
        case XEN_ERR_EXPECTED_EXPRESSION:
            return "XEN_ERR_EXPECTED_EXPRESSION";
        case XEN_ERR_INVALID_ARGS:
            return "XEN_ERR_INVALID_ARGS";
        default:
            return "UNKNOWN_ERROR";
    }
}

noreturn inline static void xen_panic(int code, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf(COLOR_BOLD COLOR_RED "(panicked) " COLOR_RESET);
    vprintf(fmt, args);
    printf("\nexited with code %d (%s)\n", code, xen_exit_code_to_str(code));
    va_end(args);
    quick_exit(code);
}

inline static void xen_runtime_error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf(COLOR_BOLD COLOR_RED "(runtime error) " COLOR_RESET);
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
}

#endif
