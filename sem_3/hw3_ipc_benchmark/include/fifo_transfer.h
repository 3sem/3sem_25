#ifndef FIFO_TRANSFER_H_
#define FIFO_TRANSFER_H_

#include "common.h"

#define FIFO_NAME "/tmp/file_transfer_fifo"

void fifo_copy_file (const char *filename);

#endif