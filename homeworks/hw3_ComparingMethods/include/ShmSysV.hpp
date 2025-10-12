#pragma once

#define BILLION		1000000000L

#define FILE_NAME	"file.txt"
#define FILE_NEW	"new_"
#define FILE_CREATE 	"time dd if=/dev/urandom of=" FILE_NAME " bs=1048576 count=1"

#define SHM_SIZE	8192
#define SHM_PERMISSIONS 0644

struct SharedMemory {
	char buf[SHM_SIZE];
	size_t buf_size;
	char eof;
};