#include "common.h"

void start_timing(struct timeval *start) {
    gettimeofday(start, NULL);
}

double stop_timing(struct timeval *start) {
    struct timeval end;
    gettimeofday(&end, NULL);
    return (end.tv_sec - start->tv_sec) + 
           (end.tv_usec - start->tv_usec) / 1000000.0;
}