#pragma once

#include "Utils.hpp"

#define FIFO_NAME 			"fifo"
#define FIFO_PERMISSIONS	0644
#define FIFO_SIZE			16384

struct Fifo {
 	char buf[FIFO_SIZE];
};