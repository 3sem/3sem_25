#pragma once

#define MESSAGE_TEXT_SIZE 8192
#define QUEUE_PERMISSIONS 0644

#define FILE_NAME	"file.txt"
#define FILE_NEW	"new_"
#define FILE_CREATE "time dd if=/dev/urandom of=" FILE_NAME " bs=1048576 count=4096"

#define MSG_END 	"EOM"

struct message {
 	long mtype;
	char mtext[MESSAGE_TEXT_SIZE];
};