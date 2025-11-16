#ifndef MAPS_DIFF
#define MAPS_DIFF

#include <stdbool.h>
#include "parse_maps.h"

static const int Init_diff_size = 10;

enum Diff_status {
    
    DIFF_SUCCESS = 0x5AFE,
    DIFF_FAILED = 0xDEAD
};

struct Maps_diff {
    Dynamic_array added;
    Dynamic_array removed;
    Dynamic_array modified;
};

void init_maps_diff(struct Maps_diff* diff);
void free_maps_diff(struct Maps_diff* diff);
int compare_snapshots(const struct Maps_snapshot* old, const struct Maps_snapshot* new, struct Maps_diff* diff);

#endif
