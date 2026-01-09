#ifndef X_COMMON_H
#define X_COMMON_H

#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;
typedef __int128_t i128;
typedef __uint128_t u128;
typedef float f32;
typedef double f64;
typedef u8 byte;

#define XEN_ANY void*

#define XEN_TRUE 1
#define XEN_FALSE 0
#define XEN_OK 0
#define XEN_FAIL 1

#ifndef NDEBUG
    #define XEN_DEBUG_PRINT(fmt, ...) fprintf(stderr, "[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
    #define XEN_DEBUG_PRINT(fmt, ...)                                                                                  \
        do {                                                                                                           \
        } while (0)
#endif

#define XEN_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define xen_container_of(ptr, type, member)                                                                            \
    ({                                                                                                                 \
        const __typeof__(((type*)0)->member)* __mptr = (ptr);                                                          \
        (type*)((char*)__mptr - offsetof(type, member));                                                               \
    })

#define XEN_LIKELY(x) __builtin_expect(!!(x), 1)
#define XEN_UNLIKELY(x) __builtin_expect(!!(x), 0)
#define XEN_BARF_ON(condition) ((void)sizeof(char[1 - 2 * !!(condition)]))
#define XEN_UNUSED(x) (void)(x)

#define XEN_CLEANUP_FREE __attribute__((cleanup(xen_cleanup_free)))

static inline void xen_cleanup_free(void* p) {
    free(*(void**)p);
}

#define XEN_MIN(a, b)                                                                                                  \
    ({                                                                                                                 \
        __typeof__(a) _a = (a);                                                                                        \
        __typeof__(b) _b = (b);                                                                                        \
        _a < _b ? _a : _b;                                                                                             \
    })

#define XEN_MAX(a, b)                                                                                                  \
    ({                                                                                                                 \
        __typeof__(a) _a = (a);                                                                                        \
        __typeof__(b) _b = (b);                                                                                        \
        _a > _b ? _a : _b;                                                                                             \
    })

#define XEN_ALIGN_UP(x, align) (((x) + (align) - 1) & ~((align) - 1))
#define XEN_KB(n) ((u64)(n) << 10)
#define XEN_MB(n) ((u64)(n) << 20)
#define XEN_GB(n) ((u64)(n) << 30)
#define XEN_PAGESIZE (sizeof(void*))

#define XEN_STRBUF_SIZE 1024

#include "xerr.h"

#endif
