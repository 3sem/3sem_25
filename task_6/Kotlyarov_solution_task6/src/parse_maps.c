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
        return NULL;
    }
    
    char maps_path[MAX_MAPS_PATH_LEN];
    snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", pid);

    FILE *file = fopen(maps_path, "r");
    if (!file) {
        DEBUG_PRINTF("ERROR: failed to open file");
        Dynamic_array_dtor(&snapshot->maps);  
        free(snapshot);
        return NULL;
    }

    char* line;
    size_t line_len = 0;
    ssize_t read = 0;
    int line_num = 0;

    while ((read = getline(&line, &line_len, file)) != -1) {
        if (read <= 1) continue;

        if (line[read - 1] == '\n') line[read - 1] = '\0';
    
        struct Memory_map map = {};
        if (parse_maps_line(line, read, &map) == PARSE_SUCCESS) {
            Dynamic_array_push_back(&snapshot->maps, &map, sizeof(map));
            ++line_num; 
        } 
    }
    
    free(line);

    if (ferror(file) || line_num == 0) {
        fclose(file);
        DEBUG_PRINTF("ERROR: getline failed");
        Dynamic_array_dtor(&snapshot->maps);
        free(snapshot);
        return NULL;
    }
    
    fclose(file);
    return snapshot;
}

int parse_maps_line(const char *line, size_t line_len, struct Memory_map* map) {
    if (!line || !map) {
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
    char *saveptr = NULL;
    char *token = strtok_r(line_copy, " ", &saveptr);

    if (!token || parse_map_addresses(token, map) == PARSE_FAILED) {
        result = PARSE_FAILED;
        goto cleanup;
    }

    token = strtok_r(NULL, " ", &saveptr);
    if (!token || parse_permissions(token, map) == PARSE_FAILED) {
        result = PARSE_FAILED;
        goto cleanup;
    }

    token = strtok_r(NULL, " ", &saveptr);
    if (!token || parse_offset(token, map) == PARSE_FAILED) {
        result = PARSE_FAILED;
        goto cleanup;
    }

    token = strtok_r(NULL, " ", &saveptr);
    if (!token || parse_devices(token, map) == PARSE_FAILED) {
        result = PARSE_FAILED;
        goto cleanup;
    }
    
    token = strtok_r(NULL, " ", &saveptr);
    if (!token || parse_inode(token, map) == PARSE_FAILED) {
        result = PARSE_FAILED;
        goto cleanup;
    }

    token = strtok_r(NULL, "", &saveptr);
    if (token) {
        parse_pathname(token, map);
    } else {
        map->pathname[0] = '\0';
    }

cleanup:
    free(line_copy);
    return result;    
}

int parse_map_addresses(char* addr_range, struct Memory_map* map) {
    if (!addr_range || !map) {
        DEBUG_PRINTF("ERROR: nullptr in parse_map_addresses\n");
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

int parse_permissions(char* perms, struct Memory_map* map) {
    if (!perms || !map) {
        DEBUG_PRINTF("ERROR: nullptr in parse_map_addresses\n");
        return PARSE_FAILED;
    }

    if (strlen(perms) != 4) {
        DEBUG_PRINTF("ERROR: invalid  permissions length: %s\n", perms);
        return PARSE_FAILED;
    }

    for (int i = 0; i < 4; i++) {
        if (i < 3) {
            if (perms[i] != 'r' && perms[i] != 'w' && perms[i] != 'x' && perms[i] != '-') 
                return PARSE_FAILED;
        } else {
            if (perms[i] != 'p' && perms[i] != 's') 
                return PARSE_FAILED;
        }
    }

    strncpy(map->permissions, perms, 4);
    map->permissions[4] = '\0';
    return PARSE_SUCCESS;
}

int parse_offset(char* offset_str, struct Memory_map* map) {
    if (!offset_str || !map) {
        DEBUG_PRINTF("ERROR: nullptr in parse_offset\n");
        return PARSE_FAILED;
    }

    char* endptr;
    map->offset = strtoul(offset_str, &endptr, 16);
    if (endptr == offset_str || *endptr != '\0') {
        DEBUG_PRINTF("ERROR: invalid offset: %s\n", offset_str);
        return PARSE_FAILED;
    }

    return PARSE_SUCCESS;
}

int parse_devices(char* dev_str, struct Memory_map* map) {
    if (!dev_str || !map) {
        DEBUG_PRINTF("ERROR: nullptr in parse_devices\n");
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

int parse_inode(char* inode_str, struct Memory_map* map) {
    if (!inode_str || !map) {
        DEBUG_PRINTF("ERROR: nullptr in parse_inode\n");
        return PARSE_FAILED;
    }

    char* endptr;
    map->inode = strtoul(inode_str, &endptr, 10);
    if (endptr == inode_str || (*endptr != '\0' && *endptr != ' ')) {
        DEBUG_PRINTF("ERROR: invalid inode: %s\n", inode_str);
        return PARSE_FAILED;
    }
    
    return PARSE_SUCCESS;
}

int parse_pathname(char* pathname, struct Memory_map* map) {
    if (pathname) {
        while (*pathname == ' ') pathname++;

        if (*pathname != '\0') {
            size_t path_len = strlen(pathname);
            if (path_len >= MAX_PATH_LEN) {
                DEBUG_PRINTF("WARNIN: pathname is too large, will be cut off: %s\n", pathname);
                path_len = MAX_PATH_LEN - 1;
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

unsigned long get_current_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}
