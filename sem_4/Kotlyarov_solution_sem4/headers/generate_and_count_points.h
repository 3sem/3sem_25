#ifndef GEN_AND_CNT_PTS
#define GEN_AND_CNT_PTS

#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include "Debug_printf.h"

typedef struct {
    size_t rows;
    size_t cols;
} DivisorsPair;

typedef struct {

    pthread_t tid;
    double a;
    double b;
    double M;
    double m;
    int num_of_points;
    int* points_count;
} MC_thread_data;

static const int Base_seed = 0xBa5e5eed;     // BaseSeed by hex)
void init_seed_for_thread(pthread_t tid, unsigned short xsubi[3]);
double generate_double(double a, double b, unsigned short xsubi[3]);
DivisorsPair find_closest_divisors(size_t n);
void* generate_and_count_points(void* arg);
double f(double x);
void append_to_csv(const char* filename, int n_threads, double time_taken);
#endif
