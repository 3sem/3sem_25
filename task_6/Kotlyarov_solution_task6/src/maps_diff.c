#include "maps_diff.h"
#include <string.h>

bool init_maps_diff(struct Maps_diff* diff) {
    if (!diff) {
        return false;
    }
    
    if (!Dynamic_array_ctor(&diff->added, Init_diff_size, sizeof(struct Memory_map)) ||
        !Dynamic_array_ctor(&diff->removed, Init_diff_size, sizeof(struct Memory_map)) ||
        !Dynamic_array_ctor(&diff->modified, Init_diff_size, sizeof(struct Memory_map))) {
        return false;
    }

    return true;
}

void free_maps_diff(struct Maps_diff* diff) {
    if (!diff) return;
    
    Dynamic_array_dtor(&diff->added);
    Dynamic_array_dtor(&diff->removed);
    Dynamic_array_dtor(&diff->modified);
}

int is_same_source(const struct Memory_map* a, const struct Memory_map* b) {
    if (!a || !b) return 0;

    return (strcmp(a->dev, b->dev) == 0 &&
            a->inode == b->inode &&
            strcmp(a->pathname, b->pathname) == 0);
}

enum Mapping_type get_mapping_type(const struct Memory_map* map) {
    if (!map) return MAPPING_ANON;
    
    if (strlen(map->pathname) > 0) {
        if (strcmp(map->pathname, "[heap]") == 0) return MAPPING_HEAP;
        if (strcmp(map->pathname, "[stack]") == 0) return MAPPING_STACK;
        if (strcmp(map->pathname, "[vvar]") == 0) return MAPPING_VVAR;
        if (strcmp(map->pathname, "[vdso]") == 0) return MAPPING_VDSO;
        if (strcmp(map->pathname, "[vsyscall]") == 0) return MAPPING_VSYSCALL;
        
        return MAPPING_FILE;
    }
    
    return MAPPING_ANON;
}

int is_inplace_modification(const struct Memory_map* old_map, const struct Memory_map* new_map) {
    if (!old_map || !new_map) return 0;

    enum Mapping_type old_type = get_mapping_type(old_map);
    enum Mapping_type new_type = get_mapping_type(new_map);

    if (old_type != new_type) return 0;

    // check necessary conditions for in-place modification
    int is_same_source_and_base = 0;
    
    if (old_type == MAPPING_FILE) {
        // for files: same source and same base address + offset
        is_same_source_and_base = (is_same_source(old_map, new_map) &&
                                   old_map->start_addr == new_map->start_addr &&
                                   old_map->offset == new_map->offset);
    }
    else if (old_type == MAPPING_HEAP || old_type == MAPPING_STACK || 
             old_type == MAPPING_VDSO || old_type == MAPPING_VVAR || 
             old_type == MAPPING_VSYSCALL) {
        // for special regions: same type and same base address
        is_same_source_and_base = (strcmp(old_map->pathname, new_map->pathname) == 0 &&
                                   old_map->start_addr == new_map->start_addr);
    }
    else if (old_type == MAPPING_ANON) {
        // for anonymous memory: same base address
        is_same_source_and_base = (old_map->start_addr == new_map->start_addr);
    }

    // sufficient condition: necessary conditions met AND mapping actually changed
    return is_same_source_and_base && 
           memcmp(old_map, new_map, sizeof(struct Memory_map)) != 0;
}

int is_unmap_remap(const struct Memory_map* old_map, const struct Memory_map* new_map) {
    if (!old_map || !new_map) return 0;

    enum Mapping_type old_type = get_mapping_type(old_map);
    enum Mapping_type new_type = get_mapping_type(new_map);

    if (old_type != new_type) return 0;

    if (old_type == MAPPING_FILE) { // unmap-remap if: same source but different base address or offset
        return (is_same_source(old_map, new_map) &&
                (old_map->start_addr != new_map->start_addr ||
                 old_map->offset != new_map->offset));
    }
    else if (old_type == MAPPING_HEAP || old_type == MAPPING_STACK || 
             old_type == MAPPING_VDSO || old_type == MAPPING_VVAR || 
             old_type == MAPPING_VSYSCALL) { // for special regions: offset is always 0, so check only base address
        return (strcmp(old_map->pathname, new_map->pathname) == 0 &&
                old_map->start_addr != new_map->start_addr);
    }
    else if (old_type == MAPPING_ANON) { // for anonymous memory: different base address but similar size
        size_t old_size = old_map->end_addr - old_map->start_addr;
        size_t new_size = new_map->end_addr - new_map->start_addr;
        double size_ratio = (double)old_size / new_size;
        
        return (old_map->start_addr != new_map->start_addr &&
                size_ratio > 0.5 && size_ratio < 2.0);
    }

    return 0;
}

int compare_snapshots(const struct Maps_snapshot* old, const struct Maps_snapshot* new, struct Maps_diff* diff) {
    if (!old || !new || !diff) return DIFF_FAILED;
    if (!init_maps_diff(diff)) return DIFF_FAILED;
    
    int* old_matched = calloc(old->maps.size, sizeof(int));
    int* new_matched = calloc(new->maps.size, sizeof(int));
    
    if (!old_matched || !new_matched) {
        free(old_matched);
        free(new_matched);
        free_maps_diff(diff);
        return DIFF_FAILED;
    }
    
    // find exact matches 
    for (size_t i = 0; i < new->maps.size / sizeof(struct Memory_map); i++) {
        struct Memory_map* new_map = safe_get_mapping(&new->maps, i);
        
        for (size_t j = 0; j < old->maps.size / sizeof(struct Memory_map); j++) {
            if (old_matched[j]) continue;
            
            struct Memory_map* old_map = safe_get_mapping(&old->maps, j);
            //DEBUG_PRINTF("new index = %zd\n", i); 
            //DEBUG_PRINTF("old index = %zd\n", j); 
            if (memcmp(old_map, new_map, sizeof(struct Memory_map)) == 0) {
                old_matched[j] = 1;
                new_matched[i] = 1;
                break;
            }
        }
    }
    
    // find in-place modifications
    for (size_t i = 0; i < new->maps.size / sizeof(struct Memory_map); i++) {
        if (new_matched[i]) continue;
        
        struct Memory_map* new_map = safe_get_mapping(&new->maps, i);
        
        for (size_t j = 0; j < old->maps.size / sizeof(struct Memory_map); j++) {
            if (old_matched[j]) continue;
            
            struct Memory_map* old_map = safe_get_mapping(&old->maps, j);
            
            if (is_inplace_modification(old_map, new_map)) {
                old_matched[j] = 1;
                new_matched[i] = 1;
                if (!Dynamic_array_push_back(&diff->modified, new_map, sizeof(struct Memory_map))) {
                    return DIFF_FAILED;
                }
                break;
            }
        }
    }
    
    // find unmap-remap pairs
    for (size_t i = 0; i < new->maps.size / sizeof(struct Memory_map); i++) {
        if (new_matched[i]) continue;
        
        struct Memory_map* new_map = safe_get_mapping(&new->maps, i);
        
        for (size_t j = 0; j < old->maps.size / sizeof(struct Memory_map); j++) {
            if (old_matched[j]) continue;
            
            struct Memory_map* old_map = safe_get_mapping(&old->maps, j);
            
            if (is_unmap_remap(old_map, new_map)) {
                old_matched[j] = 1;
                new_matched[i] = 1;
                Dynamic_array_push_back(&diff->removed, old_map, sizeof(struct Memory_map));
                Dynamic_array_push_back(&diff->added, new_map, sizeof(struct Memory_map));
                break;
            }
        }
    }
    
    for (size_t i = 0; i < new->maps.size / sizeof(struct Memory_map); i++) {
        if (!new_matched[i]) {
            struct Memory_map* new_map = safe_get_mapping(&new->maps, i);
            if (!Dynamic_array_push_back(&diff->added, new_map, sizeof(struct Memory_map))) {
                    return DIFF_FAILED;
            }
        }
    }
    
    for (size_t j = 0; j < old->maps.size / sizeof(struct Memory_map); j++) {
        if (!old_matched[j]) {
            struct Memory_map* old_map = safe_get_mapping(&old->maps, j);
            Dynamic_array_push_back(&diff->removed, old_map, sizeof(struct Memory_map));
        }
    }
    
    free(old_matched);
    free(new_matched);
    return DIFF_SUCCESS;
}
