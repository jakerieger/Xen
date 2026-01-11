#include "xenb.h"
#include "xbin_writer.h"

#include "../xen/object/xobj_function.h"
#include "../xen/object/xobj_native_function.h"
#include "../xen/object/xobj_string.h"
#include "../xen/xchunk.h"
#include "../xen/xutils.h"
#include "../xen/xcompiler.h"
#include "../xen/xtypeid.h"

/*
 * Bytecode Binary Format (.xenb)
 *
 * MAGIC             : 'XENB'    - 4 bytes
 * Version           : u8        - 1 byte
 * Lines             : u32       - 4 bytes
 * Entrypoint Length : u32       - 4 bytes
 * Entrypoint        : <string>  - n bytes
 * Args Count        : u32       - 4 bytes
 * Constants Size    : u32       - 4 bytes
 * Constants Table   : entry[]   - n bytes
 *   Constant Entry
 *     Type          : u8        - 1 byte
 *     Value Length  : u32       - 4 bytes
 *     Value         : <any>     - n bytes
 * Bytecode Size     : u32       - 4 bytes
 * Bytecode          : u8[]      - n bytes
 */

#define WRITE(v) xen_bin_write(&w, v)

void xenb_compile(const char* filename, u8* bytecode_out, size_t* size_out) {
    char* source     = xen_read_file(filename);
    xen_obj_func* fn = xen_compile(source);

    if (!fn) {
        xen_panic(XEN_ERR_EXEC_COMPILE, "failed to compile");
    }

    // write function metadata (constants, line count)
    xen_bin_writer w;
    xen_bin_writer_init(&w, 1, true);

    const u8 version            = 1;
    const u32 line_count        = (u32)(*fn->chunk.lines);
    const u32 args_count        = (u32)fn->arity;
    const u32 constants_size    = (u32)fn->chunk.constants.count;
    const char* entrypoint_name = fn->name->str;
    const u32 entrypoint_length = (u32)fn->name->length;

    assert(WRITE((i8)'X'));
    assert(WRITE((i8)'E'));
    assert(WRITE((i8)'N'));
    assert(WRITE((i8)'B'));
    assert(WRITE(version));
    assert(WRITE(line_count));
    assert(WRITE(entrypoint_name));
    assert(WRITE(args_count));
    assert(WRITE(constants_size));

    /*
     * Constant Entry
     *   Type          : u8        - 1 byte
     *   Value Length  : u32       - 4 bytes
     *   Value         : <any>     - n bytes
     */
    for (int i = 0; i < fn->chunk.constants.count; i++) {
        xen_value constant = fn->chunk.constants.values[i];
        u8 typeid          = (u8)xen_typeid_get(constant);
        assert(WRITE(typeid));
        switch (typeid) {
            case TYPEID_BOOL: {
                assert(WRITE((u32)sizeof(bool)));
                assert(WRITE(constant.as.boolean));
            } break;
            case TYPEID_NUMBER: {
                assert(WRITE((u32)sizeof(f64)));
                assert(WRITE(constant.as.number));
            } break;
            case TYPEID_STRING: {
                xen_obj_str* str = OBJ_AS_STRING(constant);
                assert(WRITE((u32)str->length));
                assert(WRITE(str->str));
            } break;
            case TYPEID_NULL: {
                assert(WRITE((u32)sizeof(bool)));
                assert(WRITE(constant.as.boolean));
            } break;
        }
    }

    assert(xen_bin_write_bytes(&w, fn->chunk.code, fn->chunk.capacity));

    *size_out = w.capacity;
    memcpy(bytecode_out, w.data, w.capacity);

    free(source);
    xen_bin_writer_free(&w);
}

#undef WRITE