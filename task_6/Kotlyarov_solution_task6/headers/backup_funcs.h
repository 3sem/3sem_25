#ifndef BACKUP_FUNCS
#define BACKUP_FUNCS

#include <sys/stat.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include "parse_maps.h"
#include "maps_diff.h"

#define BACKUP_DIR "backups"
#define FULL_BACKUP_SUBDIR "full"
#define INCREMENTAL_BACKUP_SUBDIR "incremental"
#define LOGS_DIR "logs"

enum Backup_status {
    
    BACKUP_SUCCESS = 0x5AFE,
    BACKUP_FAILED = 0xDEAD
};
int create_backup_dirs(void);
int save_full_backup(const struct Maps_snapshot* snapshot, pid_t pid);
int save_incremental_backup(const struct Maps_diff* diff, pid_t pid);
void log_message(const char* format, ...);

#endif
