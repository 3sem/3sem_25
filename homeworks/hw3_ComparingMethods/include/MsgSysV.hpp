#pragma once

#include "Utils.hpp"

#define MSQ_SIZE 		8192
#define MSQ_PERMISSIONS 0644
#define MSQ_END 		"EOM"

struct message {
 	long mtype;
	char mtext[MSQ_SIZE];
};