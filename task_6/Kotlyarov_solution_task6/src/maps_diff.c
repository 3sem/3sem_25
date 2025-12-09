#include "maps_diff.h"
#include <string.h>

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

void store_diff_history(struct Daemon_cfg* config, const struct Maps_diff* diff) {
    if (!config || !diff) return;

    size_t added_count = diff->added.size / sizeof(struct Memory_map);
    size_t removed_count = diff->removed.size / sizeof(struct Memory_map);
    size_t modified_count = diff->modified.size / sizeof(struct Memory_map);
    
    if (added_count == 0 && removed_count == 0 && modified_count == 0) {
        log_message("Skipping empty diff (no changes)");
        return;
    }
    
    log_message("Storing diff to history: +%zu, -%zu, ~%zu", 
                added_count, removed_count, modified_count);

    int index = (config->diff_history_start + config->diff_history_count) % MAX_DIFF_HISTORY;

    if (config->diff_history_count == MAX_DIFF_HISTORY) {
        free_stored_diff(&config->diff_history[index]);
    }

    if (!copy_maps_diff(diff, &config->diff_history[index].diff)) {
        log_message("ERROR: Failed to copy diff for history");
        return;
    }

    config->diff_history[index].timestamp = time(NULL);
    struct tm* tm_info = localtime(&config->diff_history[index].timestamp);
    strftime(config->diff_history[index].timestamp_str,
             sizeof(config->diff_history[index].timestamp_str),
             "%Y-%m-%d %H:%M:%S", tm_info);

    if (config->diff_history_count < MAX_DIFF_HISTORY) {
        config->diff_history_count++;
    } else {
        config->diff_history_start = (config->diff_history_start + 1) % MAX_DIFF_HISTORY;
    }
    
    log_message("History count: %d, start: %d", 
                config->diff_history_count, config->diff_history_start);
}

struct Stored_diff* get_diff_from_history(struct Daemon_cfg* config, int index) {
    if (!config || index < 1 || index > config->diff_history_count) {
        log_message("ERROR: Invalid diff index %d (count: %d)", index, config->diff_history_count);
        return NULL;
    }

    int actual_index = (config->diff_history_start + config->diff_history_count - index) % MAX_DIFF_HISTORY;
    
    struct Stored_diff* stored_diff = &config->diff_history[actual_index];
    size_t added_count = stored_diff->diff.added.size / sizeof(struct Memory_map);
    size_t removed_count = stored_diff->diff.removed.size / sizeof(struct Memory_map);
    size_t modified_count = stored_diff->diff.modified.size / sizeof(struct Memory_map);
    
    log_message("Retrieving diff %d (actual index %d): +%zu, -%zu, ~%zu", 
                index, actual_index, added_count, removed_count, modified_count);
    
    return stored_diff;
}

void print_memory_map(const struct Memory_map* map, int index) {
    if (!map) return;

    printf("  [%d] 0x%lx-0x%lx %s %lx %s %lu",
           index,
           map->start_addr,
           map->end_addr,
           map->permissions,
           map->offset,
           map->dev,
           map->inode);

    if (map->pathname[0] != '\0') {
        printf(" %s", map->pathname);
    }
    printf("\n");
}

void print_diff_details(const struct Stored_diff* stored_diff) {
    if (!stored_diff) return;

    printf("\n=== Diff at %s ===\n", stored_diff->timestamp_str);

    size_t added_count = stored_diff->diff.added.size / sizeof(struct Memory_map);
    if (added_count > 0) {
        printf("Added mappings (%zu):\n", added_count);
        for (size_t i = 0; i < added_count; i++) {
            struct Memory_map* map = safe_get_mapping(&stored_diff->diff.added, i);
            print_memory_map(map, i + 1);
        }
    }

    size_t removed_count = stored_diff->diff.removed.size / sizeof(struct Memory_map);
    if (removed_count > 0) {
        printf("Removed mappings (%zu):\n", removed_count);
        for (size_t i = 0; i < removed_count; i++) {
            struct Memory_map* map = safe_get_mapping(&stored_diff->diff.removed, i);
            print_memory_map(map, i + 1);
        }
    }

    size_t modified_count = stored_diff->diff.modified.size / sizeof(struct Memory_map);
    if (modified_count > 0) {
        printf("Modified mappings (%zu):\n", modified_count);
        for (size_t i = 0; i < modified_count; i++) {
            struct Memory_map* map = safe_get_mapping(&stored_diff->diff.modified, i);
            print_memory_map(map, i + 1);
        }
    }

    if (added_count == 0 && removed_count == 0 && modified_count == 0) {
        printf("No changes detected\n");
    }
}

void print_diff_history(struct Daemon_cfg* config) {
    if (!config || config->diff_history_count == 0) {
        printf("No diff history available (only diffs with real changes are stored)\n");
        return;
    }

    printf("Diff history (latest %d non-empty diffs of max %d):\n",
           config->diff_history_count, MAX_DIFF_HISTORY);

    for (int i = 0; i < config->diff_history_count; i++) {
        int index = (config->diff_history_start + i) % MAX_DIFF_HISTORY;
        struct Stored_diff* stored_diff = &config->diff_history[index];

        size_t added_count = stored_diff->diff.added.size / sizeof(struct Memory_map);
        size_t removed_count = stored_diff->diff.removed.size / sizeof(struct Memory_map);
        size_t modified_count = stored_diff->diff.modified.size / sizeof(struct Memory_map);

        printf("  [%d] %s (+%zu, -%zu, ~%zu)\n",
               i + 1,
               stored_diff->timestamp_str,
               added_count, removed_count, modified_count);
    }
}

int copy_maps_diff(const struct Maps_diff* src, struct Maps_diff* dst) {
    if (!src || !dst) return 0;

    size_t map_size = sizeof(struct Memory_map);
    size_t needed_capacity = (src->added.size + src->removed.size + src->modified.size) / map_size;
    
    if (!Dynamic_array_ctor(&dst->added, (needed_capacity + 10) * map_size, 0) ||
        !Dynamic_array_ctor(&dst->removed, (needed_capacity + 10) * map_size, 0) ||
        !Dynamic_array_ctor(&dst->modified, (needed_capacity + 10) * map_size, 0)) {
        return 0;
    }

    if (src->added.size > 0) {
        memcpy(dst->added.data, src->added.data, src->added.size);
        dst->added.size = src->added.size;
    }

    if (src->removed.size > 0) {
        memcpy(dst->removed.data, src->removed.data, src->removed.size);
        dst->removed.size = src->removed.size;
    }

    if (src->modified.size > 0) {
        memcpy(dst->modified.data, src->modified.data, src->modified.size);
        dst->modified.size = src->modified.size;
    }

    return 1;
}

void free_stored_diff(struct Stored_diff* stored_diff) {
    if (!stored_diff) return;
    free_maps_diff(&stored_diff->diff);
}

void free_diff_history(struct Daemon_cfg* config) {
    if (!config) return;

    for (int i = 0; i < config->diff_history_count; i++) {
        int index = (config->diff_history_start + i) % MAX_DIFF_HISTORY;
        free_stored_diff(&config->diff_history[index]);
    }

    config->diff_history_count = 0;
    config->diff_history_start = 0;
}

void log_message(const char* format, ...) {
    char logfile[256];
    snprintf(logfile, sizeof(logfile), "%s/monitor.log", LOGS_DIR);
    
    FILE* file = fopen(logfile, "a");
    if (!file) return;
    
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    
    fprintf(file, "[%04d-%02d-%02d %02d:%02d:%02d] ",
           tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
           tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
    
    va_list args;
    va_start(args, format);
    vfprintf(file, format, args);
    va_end(args);
    
    fprintf(file, "\n");
    fclose(file);
    
    printf("[LOG] ");
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}
