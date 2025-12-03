#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdint.h>

#include "montecarlo.h"
#include "common.h"

double function (double x) {
    return x*x*x;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        ERR_Y("requre one parameter - num of threads\n");
        return 0;
    }

    struct timeval time_start, time_end, prog_time;

    integral_t *cell = ctor_integration_cell(function, 0, 1000000, atoi(argv[1]), 1000000000);

    gettimeofday(&time_start, NULL);

    integrate(cell);
    join_integration(cell);
    double res = 0;
    get_result(cell, &res);

    gettimeofday(&time_end, NULL);

    dtor_integration_cell(cell);

    printf("result of integration: %lg\n", res);
    timersub(&time_end, &time_start, &prog_time);
    printf("time used to get calculate integral: "
        "\033[35m" "%ld s %ld us\n" "\033[0m",
        (uint64_t)prog_time.tv_sec, (uint64_t)prog_time.tv_usec
    );
    return 0;
}
