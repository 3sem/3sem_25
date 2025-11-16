#ifndef DAEMON_CFG
#define DAEMON_CFG

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include "Debug_printf.h"
#include "Dynamic_array_funcs.h"

#define PATH_MAX 256

enum Mode {

    NO_MODE = 0x0,
    INTERACTIVE_MODE = 0x1,
    DAEMON_MODE = 0x666
};

enum Parse_status {
    
    PARSE_SUCCESS = 0x5AFE,
    PARSE_FAILED = 0xDEAD
};

struct Memory_map {
    unsigned long start_addr;
    unsigned long end_addr;
    char permissions[5];
    unsigned long offset;
    char dev[8];
    unsigned long inode;
    char pathname[PATH_MAX];
};

struct Maps_snapshot {
    Dynamic_array maps;
    time_t timestamp;
};

struct Daemon_cfg {
    pid_t target_pid;
    useconds_t sample_interval; // us
    int is_running;
    int mode;
    char backup_dir[PATH_MAX];
    struct Maps_snapshot* last_snapshot;
};
#endif
