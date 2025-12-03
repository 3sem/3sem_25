#ifndef MONTECARLO_H
#define MONTECARLO_H

#include <pthread.h>

typedef double (*func_t)(double);
typedef struct integrate_cell_t integral_t;

#define MAX_THREADS 20
#define MAX_POINTS 1000000000

enum status_t {
    DEAD = 0,
    READY = 1,
    INTEGRATING = 2,
    FINISHED = 3
};

struct integrate_cell_t {
    func_t function;
    double lower_border, upper_border;
    double result_value;
    int threads_num, segments_per_thread;
    int threads_ready;
    pthread_t threads[MAX_THREADS];
    pthread_mutex_t mutex;
    enum status_t status;
};

integral_t *ctor_integration_cell(func_t function, double lower_border, double upper_border, int threads_num, int total_segments);
int dtor_integration_cell(integral_t *icell);

int integrate(integral_t *icell);
int get_result(integral_t *icell, double *res);
int join_integration(integral_t *icell);

#endif
