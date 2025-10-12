#pragma once

#include "Utils.hpp"

#define SHM_SIZE		8192
#define SHM_PERMISSIONS 0644

struct SharedMemory {
	char buf[SHM_SIZE];
	size_t buf_size;
	char eof;
} __attribute__((aligned(4096)));