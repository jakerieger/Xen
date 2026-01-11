#ifndef X_BIN_WRITER_H
#define X_BIN_WRITER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    uint8_t* data;
    size_t capacity;
    size_t position;
    bool auto_grow;  // Automatically resize when capacity is exceeded
    bool error;      // Track if an error occurred
} xen_bin_writer;

// Initialize writer with optional auto-grow
void xen_bin_writer_init(xen_bin_writer* writer, size_t capacity, bool auto_grow);
void xen_bin_writer_free(xen_bin_writer* writer);

// Check if writer has encountered an error
bool xen_bin_writer_has_error(const xen_bin_writer* writer);
void xen_bin_writer_clear_error(xen_bin_writer* writer);

// Get current position and size
size_t xen_bin_writer_position(const xen_bin_writer* writer);
size_t xen_bin_writer_size(const xen_bin_writer* writer);
size_t xen_bin_writer_remaining(const xen_bin_writer* writer);

// Position control
bool xen_bin_writer_seek(xen_bin_writer* writer, size_t position);
void xen_bin_writer_reset(xen_bin_writer* writer);

// Reserve space (ensures capacity, grows if needed and auto_grow is enabled)
bool xen_bin_writer_reserve(xen_bin_writer* writer, size_t additional_bytes);

// Write functions - return true on success
bool xen_bin_write_u8(xen_bin_writer* writer, uint8_t value);
bool xen_bin_write_u16(xen_bin_writer* writer, uint16_t value);
bool xen_bin_write_u32(xen_bin_writer* writer, uint32_t value);
bool xen_bin_write_u64(xen_bin_writer* writer, uint64_t value);
bool xen_bin_write_i8(xen_bin_writer* writer, int8_t value);
bool xen_bin_write_i16(xen_bin_writer* writer, int16_t value);
bool xen_bin_write_i32(xen_bin_writer* writer, int32_t value);
bool xen_bin_write_i64(xen_bin_writer* writer, int64_t value);
bool xen_bin_write_f32(xen_bin_writer* writer, float value);
bool xen_bin_write_f64(xen_bin_writer* writer, double value);
bool xen_bin_write_bool(xen_bin_writer* writer, bool value);

// Write raw bytes
bool xen_bin_write_bytes(xen_bin_writer* writer, const void* data, size_t length);

// Write length-prefixed string (u32 length + string data, no null terminator)
bool xen_bin_write_string(xen_bin_writer* writer, const char* str);

// Write fixed-length string (exactly length bytes, null-padded if shorter)
bool xen_bin_write_string_fixed(xen_bin_writer* writer, const char* str, size_t length);

// Alignment helpers
bool xen_bin_write_align(xen_bin_writer* writer, size_t alignment);
size_t xen_bin_writer_align_offset(size_t offset, size_t alignment);

#define xen_bin_write(writer, value)                                                                                   \
    _Generic((value),                                                                                                  \
      bool: xen_bin_write_bool,                                                                                        \
      uint8_t: xen_bin_write_u8,                                                                                       \
      uint16_t: xen_bin_write_u16,                                                                                     \
      uint32_t: xen_bin_write_u32,                                                                                     \
      uint64_t: xen_bin_write_u64,                                                                                     \
      int8_t: xen_bin_write_i8,                                                                                        \
      int16_t: xen_bin_write_i16,                                                                                      \
      int32_t: xen_bin_write_i32,                                                                                      \
      int64_t: xen_bin_write_i64,                                                                                      \
      float: xen_bin_write_f32,                                                                                        \
      double: xen_bin_write_f64,                                                                                       \
      char: xen_bin_write_i8,                                                                                          \
      char*: xen_bin_write_string,                                                                                     \
      const char*: xen_bin_write_string)(writer, value)

#endif  // X_BIN_WRITER_H