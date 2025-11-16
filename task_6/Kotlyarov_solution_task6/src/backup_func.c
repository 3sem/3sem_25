#include "backup_funcs.h"

int create_backup_dirs(void) {
    if (mkdir(BACKUP_DIR, 0755) != 0 && errno != EEXIST) {
        perror("Failed to create backup directory");
        return BACKUP_FAILED;
    }
    
    char full_path[256];
    snprintf(full_path, sizeof(full_path), "%s/%s", BACKUP_DIR, FULL_BACKUP_SUBDIR);
    if (mkdir(full_path, 0755) != 0 && errno != EEXIST) {
        perror("Failed to create full backup directory");
        return BACKUP_FAILED;
    }
    
    snprintf(full_path, sizeof(full_path), "%s/%s", BACKUP_DIR, INCREMENTAL_BACKUP_SUBDIR);
    if (mkdir(full_path, 0755) != 0 && errno != EEXIST) {
        perror("Failed to create incremental backup directory");
        return BACKUP_FAILED;
    }
    
    if (mkdir(LOGS_DIR, 0755) != 0 && errno != EEXIST) {
        perror("Failed to create logs directory");
        return BACKUP_FAILED;
    }
    
    return BACKUP_SUCCESS;
}

int save_full_backup(const struct Maps_snapshot* snapshot, pid_t pid) {
    if (!snapshot) return BACKUP_FAILED;
    
    char filename[256];
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    
    snprintf(filename, sizeof(filename), "%s/%s/backup_%d_%04d%02d%02d_%02d%02d%02d.maps",
             BACKUP_DIR, FULL_BACKUP_SUBDIR, pid,
             tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
             tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
    
    FILE* file = fopen(filename, "w");
    if (!file) {
        perror("Failed to create full backup file");
        return BACKUP_FAILED;
    }
    
    fprintf(file, "# Full memory maps backup for PID: %d\n", pid);
    fprintf(file, "# Timestamp: %s", ctime(&now));
    fprintf(file, "# Total mappings: %zu\n\n", snapshot->maps.size);
    
    for (size_t i = 0; i < snapshot->maps.size; i++) {
        struct Memory_map* map = (struct Memory_map*)Dynamic_array_get(&snapshot->maps, i);
        if (map) {
            fprintf(file, "%lx-%lx %s %lx %s %lu",
                   map->start_addr, map->end_addr, map->permissions,
                   map->offset, map->dev, map->inode);
            
            if (map->pathname[0] != '\0') {
                fprintf(file, " %s", map->pathname);
            }
            fprintf(file, "\n");
        }
    }
    
    fclose(file);
    log_message("Full backup saved: %s", filename);
    return BACKUP_SUCCESS;
}

int save_incremental_backup(const struct Maps_diff* diff, pid_t pid) {
    if (!diff || (!diff->added.size && !diff->removed.size && !diff->modified.size)) {
        return BACKUP_SUCCESS;
    }
    
    char filename[256];
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    
    snprintf(filename, sizeof(filename), "%s/%s/diff_%d_%04d%02d%02d_%02d%02d%02d.diff",
             BACKUP_DIR, INCREMENTAL_BACKUP_SUBDIR, pid,
             tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
             tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
    
    FILE* file = fopen(filename, "w");
    if (!file) {
        perror("Failed to create incremental backup file");
        return BACKUP_FAILED;
    }
    
    fprintf(file, "# Incremental backup for PID: %d\n", pid);
    fprintf(file, "# Timestamp: %s", ctime(&now));
    fprintf(file, "# Changes: +%zu added, -%zu removed, ~%zu modified\n\n",
           diff->added.size, diff->removed.size, diff->modified.size);
    
    if (diff->added.size > 0) {
        fprintf(file, "[ADDED]\n");
        for (size_t i = 0; i < diff->added.size; i++) {
            struct Memory_map* map = (struct Memory_map*)Dynamic_array_get(&diff->added, i);
            if (map) {
                fprintf(file, "%lx-%lx %s %lx %s %lu",
                       map->start_addr, map->end_addr, map->permissions,
                       map->offset, map->dev, map->inode);
                if (map->pathname[0] != '\0') fprintf(file, " %s", map->pathname);
                fprintf(file, "\n");
            }
        }
        fprintf(file, "\n");
    }
    
    if (diff->removed.size > 0) {
        fprintf(file, "[REMOVED]\n");
        for (size_t i = 0; i < diff->removed.size; i++) {
            struct Memory_map* map = (struct Memory_map*)Dynamic_array_get(&diff->removed, i);
            if (map) {
                fprintf(file, "%lx-%lx %s %lx %s %lu",
                       map->start_addr, map->end_addr, map->permissions,
                       map->offset, map->dev, map->inode);
                if (map->pathname[0] != '\0') fprintf(file, " %s", map->pathname);
                fprintf(file, "\n");
            }
        }
        fprintf(file, "\n");
    }
    
    if (diff->modified.size > 0) {
        fprintf(file, "[MODIFIED]\n");
        for (size_t i = 0; i < diff->modified.size; i++) {
            struct Memory_map* map = (struct Memory_map*)Dynamic_array_get(&diff->modified, i);
            if (map) {
                fprintf(file, "%lx-%lx %s %lx %s %lu",
                       map->start_addr, map->end_addr, map->permissions,
                       map->offset, map->dev, map->inode);
                if (map->pathname[0] != '\0') fprintf(file, " %s", map->pathname);
                fprintf(file, "\n");
            }
        }
    }
    
    fclose(file);
    log_message("Incremental backup saved: %s (+%zu,-%zu,~%zu)", filename,
               diff->added.size, diff->removed.size, diff->modified.size);
    return BACKUP_SUCCESS;
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
