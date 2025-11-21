#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <dirent.h>

typedef enum {
    OUTPUT_TERMINAL,
    OUTPUT_SYSLOG
} output_mode_t;

typedef struct Process
{
    pid_t target_pid;
    output_mode_t output_mode;
    char* work_dir;
    char* file_name;
    char* backup_dir;
    int inotify_fd;
    int sample_cnt;
}Process;

static const char* BACKUP_DIR = "backups";
static const size_t BUF_SIZE = 1024 * (sizeof(struct inotify_event) + 16);
static const size_t SAMPLE_TIME = 5;
static const size_t MAX_LEN = 1024;


char* get_workdir_from_procfs(pid_t pid);
void make_tmp_file_for_diff(Process* process, int sample_num);
void process_events(Process* process, char* buffer, int length);
void create_directory(const char* path);
int is_dir_exists(const char* path);
char* get_backupdir_from_procfs(pid_t pid);
void specialize_backup_dir_for_file(Process* process);
void return_init_backup_dir(char* init_backup_dir, Process* process);