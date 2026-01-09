#ifndef X_UTILS_H
#define X_UTILS_H

#include "xcommon.h"
#include "xalloc.h"
#include "xchunk.h"
#include "xerr.h"
#include "xmem.h"

#include <math.h>
#include <float.h>

/// @brief Reads a given file to a string buffer
inline static char* xen_read_file(const char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        xen_panic(XEN_ERR_OPEN_FILE, "failed to open file: %s", filename);
    }

    fseek(fp, 0, SEEK_END);
    i32 fp_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    i32 input_size = fp_size + 1;
    char* buffer   = (char*)malloc(input_size);

    size_t read_bytes  = fread(buffer, 1, input_size, fp);
    buffer[read_bytes] = '\0';
    fclose(fp);

    return buffer;
}

inline static char* xen_read_line(FILE* file) {
    size_t capacity = 128;
    size_t length   = 0;
    char* line      = (char*)malloc(capacity);

    if (XEN_UNLIKELY(line == NULL)) {
        xen_panic(XEN_ERR_ALLOCATION_FAILED, "failed to allocate char buffer");
    }

    i32 c;
    while ((c = fgetc(file)) != EOF) {
        if (c == '\n')
            break;

        if (length + 1 >= capacity) {
            capacity *= 2;
            char* new_line = realloc(line, capacity);
            if (new_line == NULL) {
                free(line);
                xen_panic(XEN_ERR_ALLOCATION_FAILED, "failed to reallocate char buffer");
            }
            line = new_line;
        }

        line[length++] = c;
    }

    if (length == 0 && c == EOF) {
        free(line);
        return NULL;
    }

    line[length] = '\0';
    return line;
}

inline static xen_chunk xen_read_bytecode(const char* filename) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        xen_panic(XEN_ERR_OPEN_FILE, "failed to open file: %s", filename);
    }

    fseek(fp, 0, SEEK_END);
    i32 fp_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    u8* buffer        = (u8*)malloc(fp_size);
    size_t read_bytes = fread(buffer, 1, fp_size, fp);
    fclose(fp);

    xen_chunk chunk;
    xen_chunk_init(&chunk);
    chunk.code     = buffer;
    chunk.capacity = fp_size;
    chunk.count    = fp_size;

    return chunk;
}

/// @brief Returns the corresponding order of magnitude suffix for the given
/// byte size (Kilobytes, Megabytes, Gigabytes, etc.) and the scaled
/// value to match
inline static char* xen_bytes_order_of_magnitude(size_t size, size_t* scaled_out) {
    if (size < XEN_KB(1)) {
        *scaled_out = size;
        return "bytes";
    }

    else if (size < XEN_MB(1)) {
        *scaled_out = size / XEN_KB(1);
        return "Kb";
    }

    else if (size < XEN_GB(1)) {
        *scaled_out = size / XEN_MB(1);
        return "Mb";
    }

    else if (XEN_UNLIKELY(size < ((u64)size << 40))) {
        *scaled_out = size / XEN_GB(1);
        return "Gb";
    }

    return NULL;
}

/// @brief Simple string hash algorithm
inline static u32 xen_hash_string(const char* key, i32 length) {
    const u32 MN_1 = 2166136261u;
    const u32 MN_2 = 16777619;

    u32 hash = MN_1;
    for (i32 i = 0; i < length; i++) {
        hash ^= (u8)key[i];
        hash *= MN_2;
    }

    return hash;
}

inline static char* xen_strdup(char* src) {
    char* str;
    char* p;
    i32 len = 0;

    while (src[len])
        len++;
    str = malloc(len + 1);
    p   = str;
    while (*src)
        *p++ = *src++;
    *p = '\0';
    return str;
}

/// @brief Decodes string literals and applies escape sequences
inline static char* decode_string_literal(const char* src, i32 src_len, i32* out_len) {
    char* dst = XEN_ALLOCATE(char, src_len + 1);
    i32 i = 0, j = 0;

    while (i < src_len) {
        if (src[i] == '\\' && i + 1 < src_len) {
            i++;
            switch (src[i]) {
                case 'n':
                    dst[j++] = '\n';
                    break;
                case 't':
                    dst[j++] = '\t';
                    break;
                case 'r':
                    dst[j++] = '\r';
                    break;
                case '\\':
                    dst[j++] = '\\';
                    break;
                case '"':
                    dst[j++] = '"';
                    break;
                default:
                    dst[j++] = src[i];  // unknown escape
                    break;
            }
        } else {
            dst[j++] = src[i];
        }
        i++;
    }

    dst[j]   = '\0';
    *out_len = j;
    return dst;
}

#endif
