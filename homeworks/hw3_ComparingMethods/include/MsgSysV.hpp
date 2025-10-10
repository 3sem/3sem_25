#pragma once

#define BILLION		1000000000L

#define FILE_NAME	"file.txt"
#define FILE_NEW	"new_"
#define FILE_CREATE 	"time dd if=/dev/urandom of=" FILE_NAME " bs=1048576 count=1"

#define MSQ_SIZE 	8192
#define MSQ_PERMISSIONS 0644
#define MSQ_END 	"EOM"

struct message {
 	long mtype;
	char mtext[MSQ_SIZE];
};