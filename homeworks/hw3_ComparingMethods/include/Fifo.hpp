#pragma once

#define BILLION			1000000000L

#define FILE_NAME		"file.txt"
#define FILE_NEW		"new_"
#define FILE_CREATE 		"time dd if=/dev/urandom of=" FILE_NAME " bs=1048576 count=1"

#define FIFO_NAME 		"fifo"
#define FIFO_PERMISSIONS        0644
#define FIFO_SIZE		16384

struct Fifo {
 	char buf[FIFO_SIZE];
};