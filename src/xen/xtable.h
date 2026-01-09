#ifndef X_TABLE_H
#define X_TABLE_H

#include "xalloc.h"
#include "xcommon.h"
#include "xvalue.h"

#define TABLE_MAX_LOAD 0.75

typedef struct xen_table_entry xen_table_entry;

struct xen_table_entry {
    xen_obj_str* key;
    xen_value value;
};

typedef struct {
    u64 count;
    u64 capacity;
    xen_table_entry* entries;
} xen_table;

void xen_table_init(xen_table* table);
void xen_table_free(xen_table* table);
bool xen_table_get(const xen_table* table, xen_obj_str* key, xen_value* value);
bool xen_table_set(xen_table* table, xen_obj_str* key, xen_value value);
bool xen_table_delete(xen_table* table, xen_obj_str* key);
void xen_table_add_all(xen_table* src, xen_table* dst);
xen_obj_str* xen_table_find_str(const xen_table* table, const char* chars, i32 length, u32 hash);

#endif
