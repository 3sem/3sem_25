#ifndef MC_INT
#define MC_INT

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>
#include "Debug_printf.h"
#include "generate_and_count_points.h"

int Default_num_of_threads = 4;
int Default_num_of_points = 100000;
double Default_a = 0;
double Default_b = 1;
double Default_m = 0;
double Default_M = 1.5;
#endif
