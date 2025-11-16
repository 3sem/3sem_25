#include "maps_diff.h"

bool init_maps_diff(struct Maps_diff* diff) {
    if (!diff) {
        return false;
    }
    
    if (!Dynamic_array_ctor(&diff->added, Init_diff_size * sizeof(struct Memory_map), 0) ||
        !Dynamic_array_ctor(&diff->removed, Init_diff_size * sizeof(struct Memory_map), 0) ||
        !Dynamic_array_ctor(&diff->modified, Init_diff_size * sizeof(struct Memory_map), 0)) {
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

int compare_snapshots(const struct Maps_snapshot* old, const struct Maps_snapshot* new, struct Maps_diff* diff) {
    if (!old || !new || !diff) {
        return DIFF_FAILED;
    }

    if (!init_maps_diff(diff)) {
        return DIFF_FAILED;
    }
    
    for (size_t i = 0; i < new->maps.size; i++) {
        struct Memory_map* new_map = ((struct Memory_map*)new->maps.data) + i;
        if (!new_map) continue;
        
        int found = 0;
        for (size_t j = 0; j < old->maps.size; j++) {
            struct Memory_map* old_map = ((struct Memory_map*)old->maps.data) + j;
            if (!old_map) continue;
            
            if (old_map->start_addr == new_map->start_addr) {
                found = 1;
                if (memcmp(old_map, new_map, sizeof(struct Memory_map)) != 0) {
                    Dynamic_array_push_back(&diff->modified, new_map, sizeof(struct Memory_map));
                }
                break;
            }
        }
        
        if (!found) {
            Dynamic_array_push_back(&diff->added, new_map, sizeof(struct Memory_map));
        }
    }
    
    for (size_t i = 0; i < old->maps.size; i++) {
        struct Memory_map* old_map = ((struct Memory_map*)old->maps.data) + i;
        if (!old_map) continue;
        
        int found = 0;
        for (size_t j = 0; j < new->maps.size; j++) {
            struct Memory_map* new_map = ((struct Memory_map*)new->maps.data) + i;
            if (!new_map) continue;
            
            if (old_map->start_addr == new_map->start_addr) {
                found = 1;
                break;
            }
        }
        
        if (!found) {
            Dynamic_array_push_back(&diff->removed, old_map, sizeof(struct Memory_map));
        }
    }
    
    return 0;
}
