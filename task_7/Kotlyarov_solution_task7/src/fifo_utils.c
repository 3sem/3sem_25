#include "fifo_utils.h"

int create_directory_recursive(const char* path) {
    char tmp[256];
    char* p = NULL;
    size_t len;
    
    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    
    if (tmp[len - 1] == '/') {
        tmp[len - 1] = 0;
    }
    
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            mkdir(tmp, 0755);
            *p = '/'; 
        }
    }
    
    return mkdir(tmp, 0755);
}

int create_fifo(const char* path) {
    char dir_path[256];
    strcpy(dir_path, path);
    
    char* last_slash = strrchr(dir_path, '/');
    if (last_slash != NULL) {
        *last_slash = '\0';
        
        if (create_directory_recursive(dir_path) == -1 && errno != EEXIST) {
            perror("mkdir");
            return -1;
        }
    }
    
    if (mkfifo(path, 0666) == -1 && errno != EEXIST) {
        perror("mkfifo");
        return -1;
    }
    
    return 0;
}

int file_exists(const char* path) {
    if (!path) return 0;
    return access(path, F_OK) == 0;
}

int is_fifo(const char* path) {
    struct stat statbuf;
    if (stat(path, &statbuf) == -1) {
        return 0;
    }
    return S_ISFIFO(statbuf.st_mode);
}

static int is_directory_empty(const char* path) {
    DIR* dir = opendir(path);
    if (!dir) {
        return 0;
    }

    int is_empty = 1;
    struct dirent* entry;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            is_empty = 0;
            break;
        }
    }
    closedir(dir);

    return is_empty;
}

int remove_fifo_and_empty_dirs(const char* path) {
    if (!file_exists(path)) {
        return 0;
    }

    if (unlink(path) == -1) {
        perror("unlink");
        return -1;
    }

    char dir_path[256];
    strncpy(dir_path, path, sizeof(dir_path) - 1);
    dir_path[sizeof(dir_path) - 1] = '\0';

    char* last_slash = strrchr(dir_path, '/');
    if (last_slash != NULL) {
        *last_slash = '\0';

        if (is_directory_empty(dir_path)) {
            rmdir(dir_path);
        }
    }

    return 0;
}

void cleanup_fifo_directory(const char* base_dir) {
    DIR* dir = opendir(base_dir);
    if (!dir) {
        return;
    }

    struct dirent* entry;
    char path[1024];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(path, sizeof(path), "%s/%s", base_dir, entry->d_name);

        if (entry->d_type == DT_DIR) {
            cleanup_fifo_directory(path);
            rmdir(path);
        } else if (is_fifo(path)) {
            remove_fifo_and_empty_dirs(path);
        }
    }

    closedir(dir);
}
