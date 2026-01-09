#ifndef X_BIN_WRITER_H
#define X_BIN_WRITER_H

#include "xcommon.h"

typedef struct {
    u64 capacity;
    u8* data;
    u64 pos;
    u64 consumed;
} xen_bin_writer;

void xen_bin_writer_init(xen_bin_writer* writer, u64 capacity);
void xen_bin_writer_free(xen_bin_writer* writer);

void xen_bin_write_bool(xen_bin_writer* writer, bool value);
void xen_bin_write_i8(xen_bin_writer* writer, i8 value);
void xen_bin_write_u8(xen_bin_writer* writer, u8 value);
void xen_bin_write_i16(xen_bin_writer* writer, i16 value);
void xen_bin_write_u16(xen_bin_writer* writer, u16 value);
void xen_bin_write_i32(xen_bin_writer* writer, i32 value);
void xen_bin_write_u32(xen_bin_writer* writer, u32 value);
void xen_bin_write_i64(xen_bin_writer* writer, i64 value);
void xen_bin_write_u64(xen_bin_writer* writer, u64 value);
void xen_bin_write_f32(xen_bin_writer* writer, f32 value);
void xen_bin_write_f64(xen_bin_writer* writer, f64 value);
void xen_bin_write_str(xen_bin_writer* writer, const char* value);
void xen_bin_write_fixed_str(xen_bin_writer* writer, const char* value, size_t length);
void xen_bin_write_byte_array(xen_bin_writer* writer, u8* values, size_t count);

void xen_bin_writer_seek(xen_bin_writer* writer, i32 position);
void xen_bin_writer_reset(xen_bin_writer* writer);
void xen_bin_writer_resize(xen_bin_writer* writer, u64 new_capacity);

#define xen_bin_write(writer, value)                                                                                   \
    _Generic((value),                                                                                                  \
      bool: xen_bin_write_bool,                                                                                        \
      i8: xen_bin_write_i8,                                                                                            \
      u8: xen_bin_write_u8,                                                                                            \
      i16: xen_bin_write_i16,                                                                                          \
      u16: xen_bin_write_u16,                                                                                          \
      i32: xen_bin_write_i32,                                                                                          \
      u32: xen_bin_write_u32,                                                                                          \
      i64: xen_bin_write_i64,                                                                                          \
      u64: xen_bin_write_u64,                                                                                          \
      f32: xen_bin_write_f32,                                                                                          \
      f64: xen_bin_write_f64,                                                                                          \
      char: xen_bin_write_u8,                                                                                          \
      char*: xen_bin_write_str,                                                                                        \
      const char*: xen_bin_write_str)(writer, value)

#endif
