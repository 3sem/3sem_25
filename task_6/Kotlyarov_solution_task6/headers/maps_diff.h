#ifndef MAPS_DIFF
#define MAPS_DIFF

#include <stdbool.h>
#include <string.h>
#include "parse_maps.h"

static const int Init_diff_size = 10;

enum Diff_status {
    
    DIFF_SUCCESS = 0x5AFE,
    DIFF_FAILED = 0xDEAD
};

enum Mapping_type {
    
    NO_TYPE = 0x0,
    MAPPING_ANON = 0xA909,
    MAPPING_FILE = 0xFA11,
    MAPPING_HEAP = 0xF00D,
    MAPPING_STACK = 0x11F0,
    MAPPING_VVAR = 0x555,
    MAPPING_VDSO = 0x777,
    MAPPING_VSYSCALL = 0x888
};

struct Maps_diff {
    Dynamic_array added;
    Dynamic_array removed;
    Dynamic_array modified;
};

bool init_maps_diff(struct Maps_diff* diff);
void free_maps_diff(struct Maps_diff* diff);
int compare_snapshots(const struct Maps_snapshot* old, const struct Maps_snapshot* new, struct Maps_diff* diff);
int is_same_source(const struct Memory_map* a, const struct Memory_map* b);
int is_mapping_modified(const struct Memory_map* a, const struct Memory_map* b);
enum Mapping_type get_mapping_type(const struct Memory_map* map);
int is_mapping_changed(const struct Memory_map* old_map, const struct Memory_map* new_map);
int is_inplace_modification(const struct Memory_map* old_map, const struct Memory_map* new_map);
int is_unmap_remap(const struct Memory_map* old_map, const struct Memory_map* new_map);
int compare_snapshots(const struct Maps_snapshot* old, const struct Maps_snapshot* new, struct Maps_diff* diff);

static inline struct Memory_map* safe_get_mapping(const Dynamic_array* array, size_t index) {
    if (!array || index >= array->size / sizeof(struct Memory_map)) return NULL;
    return (struct Memory_map*)((char*)array->data + index * sizeof(struct Memory_map));
}

#endif
