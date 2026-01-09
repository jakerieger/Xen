#include "xbin_writer.h"
#include "xerr.h"

void xen_bin_writer_init(xen_bin_writer* writer, u64 capacity) {
    if (capacity == 0) {
        xen_panic(XEN_ERR_INVALID_ARGS, "tried to initialize writer with 0 capacity");
    }

    writer->capacity = capacity;
    writer->consumed = 0;
    writer->pos      = 0;
    writer->data     = (u8*)malloc(capacity);

    if (!writer->data) {
        xen_panic(XEN_ERR_ALLOCATION_FAILED, "failed to allocate bin writer buffer");
    }
}

void xen_bin_writer_free(xen_bin_writer* writer) {
    free(writer->data);
}

static void update_position_consumed(xen_bin_writer* writer, size_t size) {
    // only update the consumed size if our position is at the end of the buffer
    if (writer->pos == writer->consumed) {
        writer->consumed += size;
    }
    writer->pos += size;
}

void xen_bin_write_bool(xen_bin_writer* writer, bool value) {
    const size_t size = sizeof(bool);
    memcpy(writer->data + writer->pos, &value, size);
    update_position_consumed(writer, size);
}

void xen_bin_write_i8(xen_bin_writer* writer, i8 value) {
    const size_t size = sizeof(i8);
    memcpy(writer->data + writer->pos, &value, size);
    update_position_consumed(writer, size);
}

void xen_bin_write_u8(xen_bin_writer* writer, u8 value) {
    const size_t size = sizeof(u8);
    memcpy(writer->data + writer->pos, &value, size);
    update_position_consumed(writer, size);
}

void xen_bin_write_i16(xen_bin_writer* writer, i16 value) {
    const size_t size = sizeof(i16);
    memcpy(writer->data + writer->pos, &value, size);
    update_position_consumed(writer, size);
}

void xen_bin_write_u16(xen_bin_writer* writer, u16 value) {
    const size_t size = sizeof(u16);
    memcpy(writer->data + writer->pos, &value, size);
    update_position_consumed(writer, size);
}

void xen_bin_write_i32(xen_bin_writer* writer, i32 value) {
    const size_t size = sizeof(i32);
    memcpy(writer->data + writer->pos, &value, size);
    update_position_consumed(writer, size);
}

void xen_bin_write_u32(xen_bin_writer* writer, u32 value) {
    const size_t size = sizeof(u32);
    memcpy(writer->data + writer->pos, &value, size);
    update_position_consumed(writer, size);
}

void xen_bin_write_i64(xen_bin_writer* writer, i64 value) {
    const size_t size = sizeof(i64);
    memcpy(writer->data + writer->pos, &value, size);
    update_position_consumed(writer, size);
}

void xen_bin_write_u64(xen_bin_writer* writer, u64 value) {
    const size_t size = sizeof(u64);
    memcpy(writer->data + writer->pos, &value, size);
    update_position_consumed(writer, size);
}

void xen_bin_write_f32(xen_bin_writer* writer, f32 value) {
    const size_t size = sizeof(f32);
    memcpy(writer->data + writer->pos, &value, size);
    update_position_consumed(writer, size);
}

void xen_bin_write_f64(xen_bin_writer* writer, f64 value) {
    const size_t size = sizeof(f64);
    memcpy(writer->data + writer->pos, &value, size);
    update_position_consumed(writer, size);
}

void xen_bin_write_str(xen_bin_writer* writer, const char* value) {
    u32 len = (u32)strlen(value);
    // write string length first. format is len:string
    xen_bin_write(writer, len);
    xen_bin_write_fixed_str(writer, value, len);
}

void xen_bin_write_fixed_str(xen_bin_writer* writer, const char* value, size_t length) {
    if (strlen(value) >= length) {
        xen_panic(XEN_ERR_INVALID_ARGS, "provided string is longer than provided length");
    }
    memcpy(writer->data + writer->pos, value, length);
    update_position_consumed(writer, length);
}

void xen_bin_write_byte_array(xen_bin_writer* writer, u8* values, size_t count) {
    if (!values) {
        xen_panic(XEN_ERR_INVALID_ARGS, "values ptr is NULL");
    }
    memcpy(writer->data + writer->pos, values, count);
    update_position_consumed(writer, count);
}

void xen_bin_writer_seek(xen_bin_writer* writer, i32 position) {
    writer->pos = position;
}

void xen_bin_writer_reset(xen_bin_writer* writer) {
    free(writer->data);
    xen_bin_writer_init(writer, writer->capacity);
}

void xen_bin_writer_resize(xen_bin_writer* writer, u64 new_capacity) {
    writer->data = (u8*)realloc(writer->data, new_capacity);
}
