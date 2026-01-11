#include "xbin_writer.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

static bool ensure_space(xen_bin_writer* writer, size_t needed) {
    if (writer->error) {
        return false;
    }

    size_t required = writer->position + needed;

    if (required <= writer->capacity) {
        return true;
    }

    if (!writer->auto_grow) {
        writer->error = true;
        return false;
    }

    // Grow by at least 1.5x, or enough to fit the required size
    size_t new_capacity = writer->capacity + (writer->capacity >> 1);
    if (new_capacity < required) {
        new_capacity = required;
    }

    // Round up to nearest power of 2 for better memory allocation
    new_capacity--;
    new_capacity |= new_capacity >> 1;
    new_capacity |= new_capacity >> 2;
    new_capacity |= new_capacity >> 4;
    new_capacity |= new_capacity >> 8;
    new_capacity |= new_capacity >> 16;
    new_capacity |= new_capacity >> 32;
    new_capacity++;

    uint8_t* new_data = (uint8_t*)realloc(writer->data, new_capacity);
    if (!new_data) {
        writer->error = true;
        return false;
    }

    writer->data     = new_data;
    writer->capacity = new_capacity;
    return true;
}

void xen_bin_writer_init(xen_bin_writer* writer, size_t capacity, bool auto_grow) {
    assert(writer != NULL);
    assert(capacity > 0);

    writer->data = (uint8_t*)malloc(capacity);
    if (!writer->data) {
        writer->capacity  = 0;
        writer->position  = 0;
        writer->auto_grow = false;
        writer->error     = true;
        return;
    }

    writer->capacity  = capacity;
    writer->position  = 0;
    writer->auto_grow = auto_grow;
    writer->error     = false;
}

void xen_bin_writer_free(xen_bin_writer* writer) {
    if (writer && writer->data) {
        free(writer->data);
        writer->data     = NULL;
        writer->capacity = 0;
        writer->position = 0;
    }
}

bool xen_bin_writer_has_error(const xen_bin_writer* writer) {
    return writer->error;
}

void xen_bin_writer_clear_error(xen_bin_writer* writer) {
    writer->error = false;
}

size_t xen_bin_writer_position(const xen_bin_writer* writer) {
    return writer->position;
}

size_t xen_bin_writer_size(const xen_bin_writer* writer) {
    return writer->position;
}

size_t xen_bin_writer_remaining(const xen_bin_writer* writer) {
    return writer->capacity - writer->position;
}

bool xen_bin_writer_seek(xen_bin_writer* writer, size_t position) {
    if (position > writer->capacity) {
        writer->error = true;
        return false;
    }
    writer->position = position;
    return true;
}

void xen_bin_writer_reset(xen_bin_writer* writer) {
    writer->position = 0;
    writer->error    = false;
}

bool xen_bin_writer_reserve(xen_bin_writer* writer, size_t additional_bytes) {
    return ensure_space(writer, additional_bytes);
}

// Write primitives with bounds checking
bool xen_bin_write_u8(xen_bin_writer* writer, uint8_t value) {
    if (!ensure_space(writer, sizeof(uint8_t))) {
        return false;
    }
    writer->data[writer->position++] = value;
    return true;
}

bool xen_bin_write_u16(xen_bin_writer* writer, uint16_t value) {
    if (!ensure_space(writer, sizeof(uint16_t))) {
        return false;
    }
    memcpy(writer->data + writer->position, &value, sizeof(uint16_t));
    writer->position += sizeof(uint16_t);
    return true;
}

bool xen_bin_write_u32(xen_bin_writer* writer, uint32_t value) {
    if (!ensure_space(writer, sizeof(uint32_t))) {
        return false;
    }
    memcpy(writer->data + writer->position, &value, sizeof(uint32_t));
    writer->position += sizeof(uint32_t);
    return true;
}

bool xen_bin_write_u64(xen_bin_writer* writer, uint64_t value) {
    if (!ensure_space(writer, sizeof(uint64_t))) {
        return false;
    }
    memcpy(writer->data + writer->position, &value, sizeof(uint64_t));
    writer->position += sizeof(uint64_t);
    return true;
}

bool xen_bin_write_i8(xen_bin_writer* writer, int8_t value) {
    return xen_bin_write_u8(writer, (uint8_t)value);
}

bool xen_bin_write_i16(xen_bin_writer* writer, int16_t value) {
    return xen_bin_write_u16(writer, (uint16_t)value);
}

bool xen_bin_write_i32(xen_bin_writer* writer, int32_t value) {
    return xen_bin_write_u32(writer, (uint32_t)value);
}

bool xen_bin_write_i64(xen_bin_writer* writer, int64_t value) {
    return xen_bin_write_u64(writer, (uint64_t)value);
}

bool xen_bin_write_f32(xen_bin_writer* writer, float value) {
    if (!ensure_space(writer, sizeof(float))) {
        return false;
    }
    memcpy(writer->data + writer->position, &value, sizeof(float));
    writer->position += sizeof(float);
    return true;
}

bool xen_bin_write_f64(xen_bin_writer* writer, double value) {
    if (!ensure_space(writer, sizeof(double))) {
        return false;
    }
    memcpy(writer->data + writer->position, &value, sizeof(double));
    writer->position += sizeof(double);
    return true;
}

bool xen_bin_write_bool(xen_bin_writer* writer, bool value) {
    return xen_bin_write_u8(writer, value ? 1 : 0);
}

bool xen_bin_write_bytes(xen_bin_writer* writer, const void* data, size_t length) {
    if (!data && length > 0) {
        writer->error = true;
        return false;
    }

    if (!ensure_space(writer, length)) {
        return false;
    }

    memcpy(writer->data + writer->position, data, length);
    writer->position += length;
    return true;
}

bool xen_bin_write_string(xen_bin_writer* writer, const char* str) {
    if (!str) {
        writer->error = true;
        return false;
    }

    uint32_t length = (uint32_t)strlen(str);

    // Write length prefix
    if (!xen_bin_write_u32(writer, length)) {
        return false;
    }

    // Write string data (without null terminator)
    return xen_bin_write_bytes(writer, str, length);
}

bool xen_bin_write_string_fixed(xen_bin_writer* writer, const char* str, size_t length) {
    if (!str) {
        writer->error = true;
        return false;
    }

    if (!ensure_space(writer, length)) {
        return false;
    }

    size_t str_len = strlen(str);

    if (str_len > length) {
        writer->error = true;
        return false;
    }

    // Copy string
    memcpy(writer->data + writer->position, str, str_len);

    // Null-pad the rest
    if (str_len < length) {
        memset(writer->data + writer->position + str_len, 0, length - str_len);
    }

    writer->position += length;
    return true;
}

size_t xen_bin_writer_align_offset(size_t offset, size_t alignment) {
    return (offset + alignment - 1) & ~(alignment - 1);
}

bool xen_bin_write_align(xen_bin_writer* writer, size_t alignment) {
    size_t aligned_pos = xen_bin_writer_align_offset(writer->position, alignment);
    size_t padding     = aligned_pos - writer->position;

    if (padding == 0) {
        return true;
    }

    if (!ensure_space(writer, padding)) {
        return false;
    }

    // Write zeros for padding
    memset(writer->data + writer->position, 0, padding);
    writer->position = aligned_pos;
    return true;
}