#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "montecarlo.h"
#include "common.h"

static  void *integrate_segment(void* args);

integral_t *ctor_integration_cell(func_t function, double lower_border, double upper_border, int threads_num, int total_segments) {
    if (total_segments <= 0 || total_segments > MAX_POINTS) {
        ERR_R("ERROR: segments per tread cannot be lower 1 nor upper %d, your number: %d\n", MAX_POINTS, total_segments);
        return 0;
    }
    if (threads_num <= 0 || threads_num > MAX_THREADS) {
        ERR_R("ERROR: number of threads cannot be lower 1 nor upper %d, your number: %d\n", MAX_THREADS, threads_num);
        return 0;
    }
    if (function == NULL) {
        ERR_R("ERROR: function undefined\n");
        return 0;
    }

    integral_t *icell;
    CALLOC_SELF(icell);

    icell->lower_border = lower_border;
    icell->upper_border = upper_border;
    icell->segments_per_thread = (total_segments-1) / threads_num + 1;
    icell->threads_num = threads_num;
    icell->status = READY;
    icell->function = function;
    icell->threads_ready = 0;
    pthread_mutex_init(&icell->mutex, NULL);

    ERR_G("intergation cell created properly\n");
    return icell;
}
int dtor_integration_cell(integral_t *icell) {
    pthread_mutex_destroy(&icell->mutex);
    SET_ZERO(icell);
    free(icell);
    ERR_G("intergation cell destructed properly\n");
    return 0;
}


int integrate(integral_t *icell) {
    if (!icell) {
        ERR_R("cell not existed\n");
        return 1;
    }
    if (icell->status != READY) {
        ERR_R("ERROR: current status %d, %d required\n", icell->status, READY);
        return 1;
    }

    int i;

    pthread_mutex_lock(&icell->mutex);
    for (i = 0; i < icell->threads_num; ++i) {
        void *args = malloc(sizeof(icell) + sizeof(i));
        *(integral_t **)args = icell;
        *(int *)(args + sizeof(icell)) = i;

        if (pthread_create(icell->threads + i, NULL, integrate_segment, args)) {
            icell->status = DEAD;
            ERR_R("pthread was not created\n");
            break;
        }
    }
    pthread_mutex_unlock(&icell->mutex);

    if (icell->status == DEAD) {
        for (i = i - 1; i >= 0; --i) {
            pthread_join(icell->threads[i], NULL);
        }
        ERR_R("intergation cell is dead, no intergation implemented\n");
        return 1;
    }

    icell->status = INTEGRATING;
    ERR_G("intergation started successfully\n");
    return 0;
}

int get_result(integral_t *icell, double *res) {
    if (!icell) {
        ERR_R("cell not existed\n");
        return 1;
    }
    if (icell->status == INTEGRATING && icell->threads_num == icell->threads_ready) {
        join_integration(icell);
    }
    if (icell->status == DEAD) {
        ERR_R("cell is dead\n");
        return 1;
    }
    if (icell->status != FINISHED) {
        ERR_Y("cell is not ready\n");
        return 2;
    }

    *res = icell->result_value;
    return 0;
}

int join_integration(integral_t *icell) {
    if (!icell) {
        ERR_R("cell not existed\n");
        return 1;
    }
    if (icell->status == DEAD) {
        ERR_R("cell is dead\n");
        return 1;
    }
    if (icell->status != INTEGRATING) {
        ERR_Y("cell does not requires join\n");
        return 0;
    }

    for (int i = 0; i < icell->threads_num; ++i) {
        if(pthread_join(icell->threads[i], NULL)) {
            icell->status = DEAD;
            ERR_R("failed to integrate thread %d\n", i);
        }
    }

    if (icell->status == DEAD) {
        ERR_R("cell is dead now\n");
        return 1;
    }

    icell->status = FINISHED;
    return 0;
}

void *integrate_segment(void* args) {
    if (args == NULL) {
        ERR_R("no arguments to intergate segment\n");
        pthread_exit(1);
    }

    integral_t *icell = *(integral_t **)args;
    int number = *(int*)(args + sizeof(icell));

    free(args);

    pthread_mutex_lock(&icell->mutex);
    pthread_mutex_unlock(&icell->mutex);

    if (icell->status == DEAD) {
        ERR_R("intgration cell is dead, seg #%d\n", number);
        pthread_exit(1);
    }

    double thread_segment_len = (icell->upper_border - icell->lower_border) / icell->threads_num;
    double segment_len = thread_segment_len / icell->segments_per_thread;
    double lower_border = icell->lower_border + thread_segment_len * number;
    double thread_result_value = 0;

    for (int i = 0; i < icell->segments_per_thread; ++i) {
        thread_result_value += icell->function(lower_border + i * segment_len) * segment_len;
    }

    pthread_mutex_lock(&icell->mutex);
    icell->threads_ready++;
    icell->result_value += thread_result_value;
    pthread_mutex_unlock(&icell->mutex);

    // ERR_G("thread %d successfully finished with result %lg\n", number, thread_result_value);

    pthread_exit(0);
}
