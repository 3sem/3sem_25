#ifndef MONTECARLO_H
#define MONTECARLO_H

typedef int (*func_t)(int);

int intergate(func_t function, int lower_bound, int upper_bound);

#endif
