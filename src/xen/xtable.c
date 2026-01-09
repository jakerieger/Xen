#include "xtable.h"
#include "xmem.h"
#include "xvalue.h"
#include "xobject.h"

void xen_table_init(xen_table* table) {
    table->count    = 0;
    table->capacity = 0;
    table->entries  = NULL;
}

void xen_table_free(xen_table* table) {
    XEN_FREE_ARRAY(xen_table_entry, table->entries, table->capacity);
    xen_table_init(table);
}

static xen_table_entry* find_entry(xen_table_entry* entries, i32 capacity, xen_obj_str* key) {
    u32 index                  = key->hash % capacity;
    xen_table_entry* tombstone = NULL;

    for (;;) {
        xen_table_entry* entry = &entries[index];
        if (entry->key == NULL) {
            if (VAL_IS_NULL(entry->value)) {
                return tombstone != NULL ? tombstone : entry;
            } else {
                if (tombstone == NULL)
                    tombstone = entry;
            }
        } else if (entry->key == key) {
            return entry;
        }

        index = (index + 1) % capacity;
    }
}

static void adjust_capacity(xen_table* table, i32 capacity) {
    xen_table_entry* entries = XEN_ALLOCATE(xen_table_entry, capacity);
    for (i32 i = 0; i < capacity; i++) {
        entries[i].key   = NULL;
        entries[i].value = NULL_VAL;
    }

    table->count = 0;
    for (i32 i = 0; i < table->capacity; i++) {
        const xen_table_entry* entry = &table->entries[i];
        if (entry->key != NULL) {
            xen_table_entry* dst = find_entry(entries, capacity, entry->key);
            dst->key             = entry->key;
            dst->value           = entry->value;
            // Each time we find a non-tombstone entry, we increment count
            table->count++;
        }
    }

    XEN_FREE_ARRAY(xen_table_entry, table->entries, table->capacity);
    table->entries  = entries;
    table->capacity = capacity;
}

bool xen_table_get(const xen_table* table, xen_obj_str* key, xen_value* value) {
    if (table->count == 0)
        return XEN_FALSE;

    const xen_table_entry* entry = find_entry(table->entries, table->capacity, key);
    if (entry->key == NULL)
        return XEN_FALSE;

    *value = entry->value;
    return XEN_TRUE;
}

bool xen_table_set(xen_table* table, xen_obj_str* key, xen_value value) {
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
        const i32 capacity = XEN_GROW_CAPACITY(table->capacity);
        adjust_capacity(table, capacity);
    }

    xen_table_entry* entry = find_entry(table->entries, table->capacity, key);
    const bool is_new_key  = entry->key == NULL;

    // Include tombstones in our count
    if (is_new_key && VAL_IS_NULL(entry->value))
        table->count++;

    entry->key   = key;
    entry->value = value;

    return is_new_key;
}

bool xen_table_delete(xen_table* table, xen_obj_str* key) {
    if (table->count == 0)
        return XEN_FALSE;

    xen_table_entry* entry = find_entry(table->entries, table->capacity, key);
    if (entry->key == NULL)
        return XEN_FALSE;

    // Place tombstone in entry
    entry->key   = NULL;
    entry->value = BOOL_VAL(XEN_TRUE);

    return XEN_TRUE;
}

void xen_table_add_all(xen_table* src, xen_table* dst) {
    for (i32 i = 0; i < src->capacity; i++) {
        const xen_table_entry* entry = &src->entries[i];
        if (entry->key != NULL) {
            xen_table_set(dst, entry->key, entry->value);
        }
    }
}

xen_obj_str* xen_table_find_str(const xen_table* table, const char* chars, i32 length, u32 hash) {
    if (table->count == 0)
        return NULL;

    u32 index = hash % table->capacity;
    for (;;) {
        const xen_table_entry* entry = &table->entries[index];
        if (entry->key == NULL) {
            // Stop if we find an empty non-tombstone entry
            if (VAL_IS_NULL(entry->value))
                return NULL;
        } else if (entry->key->length == length && entry->key->hash == hash &&
                   memcmp(entry->key->str, chars, length) == 0) {
            return entry->key;
        }

        index = (index + 1) % table->capacity;
    }
}
