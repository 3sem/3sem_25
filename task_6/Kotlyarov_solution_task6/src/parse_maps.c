#include "parse_maps.h"

struct Maps_snapshot* read_maps_snapshot(pid_t pid) {
    struct Maps_snapshot* snapshot = (struct Maps_snapshot*) calloc(1, sizeof(struct Maps_snapshot));
    if (!snapshot) {
        DEBUG_PRINTF("ERROR: memory was not allocated");
        return NULL;
    }

    snapshot->timestamp = time(NULL);

    if (!Dynamic_array_ctor(&snapshot->maps, sizeof(struct Memory_map) * INIT_MAPS_COUNT, 0)) {
        free(snapshot);
        return NULL
    }
    
    char maps_path[MAX_MAPS_PATH_LEN];
    snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", pid);

    int64_t file_size = get_file_size(maps_path);
    if (file_size < 0) {
        DEBUG_PRINTF("ERROR: returned negative size");
        Dynamic_array_dtor(&snapshot->maps);  
        free(snapshot);
        return NULL;
    }
    
    FILE *file = fopen(maps_path, "r");
    if (!file) {
        perror("ERROR: failed to open file");
        return NULL;
    }

    char* line;
    size_t line_len = 0;
    ssize_t read = 0;
    int line_num = 0;
    long prev_file_pos = 0, curr_file_pos = 0;
    while ((read = getline(&line, &line_len, file)) != -1) {
        read = getline(&line, &line_len, file);
        if (read == -1) {
            DEBUG_PRINTF("ERROR: getline failed");
            return NULL;
        }

        if (read <= 1) continue;

        ++line_num; 
    
        if (parse_maps_line(line, read, &map) == PARSE_SUCCESS) {
            Dynamic_array_push_back(&snapshot->maps, &map, sizeof(map));
        } else {
            curr_file_pos = ftell(file);
            if(curr_file_pos == -1) {
                DEBUG_PRINTF("ERROR: ftell failed");
                return NULL;
            } else if (curr_file_pos == file_size) {
                break;
            } else if(curr_file_pos == prev_file_pos) {
                DEBUG_PRINTF("ERROR: getline stuck");
                return NULL;
            }
        } 
    }

    free(line);
    fclose(file);
    
    if (read == -1) {
        DEBUG_PRINTF("ERROR: getline failed");
    }

    return snapshot;
}

int parse_maps_line(const char *line, size_t line_len, Dynamic_array* maps) {
    struct Memory_map map = {};

    if (!line || !maps) {
        DEBUG_PRINTF("ERROR: nullptr in parse_maps_line_safe\n");
        return PARSE_FAILED;
    }

    if (line_len <= 0) {
        DEBUG_PRINTF("ERROR: invalid line len %zd\n", line_len);
        return PARSE_FAILED;
    }

    char* line_copy = calloc(line_len + 1, sizeof(char));
    if (!line_copy) {
        DEBUG_PRINTF("ERROR: memory was not allocated");
        return PARSE_FAILED;
    }
    memcpy(line_copy, line, line_len);

    char *newline = strchr(line_copy, '\n');
    if (newline) *newline = '\0';

    int result = PARSE_SUCCESS;

    if (parse_map_addresses(line, &map) == PARSE_FAILED) {
        result = PARSE_FAILED;
        goto cleanup;
    }

    if (parse_permissions(&map) == PARSE_FAILED) {
        result = PARSE_FAILED;
        goto cleanup;
    }

    if (parse_offset(&map) == PARSE_FAILED) {
        result = PARSE_FAILED;
        goto cleanup;
    }

    if (parse_devices(&map) == PARSE_FAILED) {
        result = PARSE_FAILED;
        goto cleanup;
    }
    
    if (parse_inode(&map) == PARSE_FAILED) {
        result = PARSE_FAILED;
        goto cleanup;
    }

    if (parse_pathname(&map) == PARSE_FAILED) {
        result = PARSE_FAILED;
        goto cleanup;
    }

    Dynamic_array_push_back(maps, &map, sizeof(map));

cleanup:
    free(line_copy);
    return result;  
}

int parse_map_addresses(char* line, struct Memory_map* map) {
    char *addr_range = strtok(line, " ");
    if (!addr_range) {
        DEBUG_PRINTF("ERROR: addresses range was not found\n");
        return PARSE_FAILED;
    }

    char *dash = strchr(addr_range, '-');
    if (!dash) {
        DEBUG_PRINTF("ERROR: invalid addresses format: %s\n", addr_range);
        return PARSE_FAILED;
    }
    *dash = '\0';  

    char *endptr;
    map->start_addr = strtoul(addr_range, &endptr, 16);
    if (endptr == addr_range || *endptr != '\0') {
        DEBUG_PRINTF("ERROR: invalid start address: %s\n", addr_range);
        return PARSE_FAILED;
    }

    map->end_addr = strtoul(dash + 1, &endptr, 16);
    if (endptr == dash + 1 || *endptr != '\0') {
        DEBUG_PRINTF("ERROR: invalid end address: %s\n", dash + 1);
        return PARSE_FAILED;
    }

    return PARSE_SUCCESS;
}

int parse_permissions(struct Memory_map* map) {
    char *perms = strtok(NULL, " ");
    if (!perms) {
        DEBUG_PRINTF("ERROR: permissions were not found\n");
        return PARSE_FAILED;
    }

    if (strlen(perms) != 4) {
        DEBUG_PRINTF("ERROR: invalid  permissions length: %s\n", perms);
        return PARSE_FAILED;
    }

    for (int i = 0; i < 4; i++) {
        if (perms[i] != 'r' && perms[i] != 'w' && perms[i] != 'x' && perms[i] != 'p' && perms[i] != '-') {
            DEBUG_PRINTF("ERROR: invalid char in permissions: %c\n", perms[i]);
            return PARSE_FAILED;
        }
    }

    strncpy(map->permissions, perms, 4);
    map->permissions[4] = '\0';

    return PARSE_SUCCESS;
}

int parse_offset(struct Memory_map* map) {
    char *offset_str = strtok(NULL, " ");
    if (!offset_str) {
        DEBUG_PRINTF("ERROR: не найден offset\n");
        return PARSE_FAILED;
    }

    char* endptr;
    map->offset = strtoul(offset_str, &endptr, 16);
    if (endptr == offset_str || *endptr != '\0') {
        DEBUG_PRINTF("ERROR: некорректный offset: %s\n", offset_str);
        return PARSE_FAILED;
    }

    return PARSE_SUCCESS;
}

int parse_devices(struct Memory_map* map) {
    char *dev_str = strtok(NULL, " ");
    if (!dev_str) {
        DEBUG_PRINTF("ERROR: devices were not found\n");
        return PARSE_FAILED;
    }

    if (strlen(dev_str) > sizeof(map->dev) - 1) {
        DEBUG_PRINTF("ERROR: device is too large: %s\n", dev_str);
        return PARSE_FAILED;
    }

    int colon_count = 0;
    for (char *p = dev_str; *p; p++) {
        if (*p == ':') colon_count++;
        else if (!isxdigit((unsigned char)*p) && *p != '-') {
            DEBUG_PRINTF("ERROR: invalid char in devices: %c\n", *p);
            return PARSE_FAILED;
        }
    }

    if (colon_count != 1) {
        DEBUG_PRINTF("ERROR: invalid format in devices: %s\n", dev_str);
        return PARSE_FAILED;
    }

    strncpy(map->dev, dev_str, sizeof(map->dev) - 1);
    map->dev[sizeof(map->dev) - 1] = '\0';
    
    return PARSE_SUCCESS;
}

int parse_inode(struct Memory_map* map) {
    char *inode_str = strtok(NULL, " ");
    if (!inode_str) {
        DEBUG_PRINTF("ERROR: не найден inode\n");
        return PARSE_FAILED;
    }

    char* endptr;
    map->inode = strtoul(inode_str, &endptr, 10);
    if (endptr == inode_str || (*endptr != '\0' && *endptr != ' ')) {
        DEBUG_PRINTF("ERROR: некорректный inode: %s\n", inode_str);
        return PARSE_FAILED;
    }
    
    return PARSE_SUCCESS;
}

int parse_pathname(struct Memory_map* map) {
    char *pathname = strtok(NULL, "");
    if (pathname) {
        while (*pathname == ' ') pathname++;

        if (*pathname != '\0') {
            size_t path_len = strlen(pathname);
            if (path_len >= MAX_PATH_LENGTH) {
                DEBUG_PRINTF("WARNIN: pathname is too large, will be cut off: %s\n", pathname);
                path_len = MAX_PATH_LENGTH - 1;
            }

            strncpy(map->pathname, pathname, path_len);
            map->pathname[path_len] = '\0';
        } else {
            map->pathname[0] = '\0';
        }
    } else {
        map->pathname[0] = '\0';
    }

    return PARSE_SUCCESS;
}

void free_maps_snapshot(struct Maps_snapshot* snapshot) {
    if (snapshot) {
        Dynamic_array_dtor(&snapshot->maps);
        free(snapshot);
    }
}
