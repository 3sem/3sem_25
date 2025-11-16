#ifndef PARSE_MAPS
#define PARSE_MAPS

#include "Daemon_config.h"
#include "GetFileSize.h"

void free_maps_snapshot(struct Maps_snapshot* snapshot)
struct Maps_snapshot* read_maps_snapshot(pid_t pid);
int parse_maps_line(const char *line, size_t line_len, Dynamic_array* maps);
int parse_map_addresses(char* line, struct Memory_map* map);
int parse_permissions(struct Memory_map* map);
int parse_offset(struct Memory_map* map);
int parse_devices(struct Memory_map* map);
int parse_inode(struct Memory_map* map);
int parse_pathname(struct Memory_map* map);
void free_maps_snapshot(struct Maps_snapshot* snapshot);

#endif
