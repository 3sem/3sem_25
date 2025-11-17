#ifndef PARSE_MAPS
#define PARSE_MAPS

#include <ctype.h>
#include <sys/time.h>
#include "Daemon_config.h"

#define INIT_MAPS_COUNT 32
#define MAX_MAPS_PATH_LEN 64
#define MAX_PATH_LEN 256
#define MAX_LINE_LENGTH 256

struct Maps_snapshot* read_maps_snapshot(pid_t pid);
int parse_maps_line(const char *line, size_t line_len, struct Memory_map* maps);
int parse_map_addresses(char* addr_range, struct Memory_map* map);
int parse_permissions(char* perms, struct Memory_map* map);
int parse_offset(char* offset_str, struct Memory_map* map);
int parse_devices(char* dev_str, struct Memory_map* map);
int parse_inode(char* inode_str, struct Memory_map* map);
int parse_pathname(char* pathname, struct Memory_map* map);
void free_maps_snapshot(struct Maps_snapshot* snapshot);
unsigned long get_current_time_ms();

#endif
