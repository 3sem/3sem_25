#include "generate_and_count_points.h"

void init_seed_for_thread(pthread_t tid, unsigned short xsubi[3]) {

    unsigned long tid_num = (unsigned long)tid;
    xsubi[0] = (Base_seed + tid_num) & 0xFFFF;
    xsubi[1] = (Base_seed * (tid_num + 1)) & 0xFFFF;
    xsubi[2] = (Base_seed ^ (tid_num << 8)) & 0xFFFF;
}

double generate_double(double a, double b, unsigned short xsubi[3]) {
    return a + (b - a) * erand48(xsubi);
}

DivisorsPair find_closest_divisors(size_t n) {
    if (n <= 0) {
        return (DivisorsPair){0, 0};
    }
    if (n == 1) {
        return (DivisorsPair){1, 1};
    }

    size_t sqrt_n = (size_t)sqrt(n);
    size_t a = 1;
    size_t b = n;

    for (size_t i = sqrt_n; i >= 1; i--) {
        if (n % i == 0) {
            a = i;
            b = n / i;
            break;
        }
    }

    return (DivisorsPair){a, b};
}

void* generate_and_count_points(void* arg) {

    MC_thread_data* t_data = (MC_thread_data*) arg;
    unsigned short xsubi[3];
    init_seed_for_thread(t_data->tid, xsubi);
    *(t_data->points_count) = 0;

    int num_of_points = t_data->num_of_points;
    int points_count = 0;
    double a = t_data->a;
    double b = t_data->b;
    double m = t_data->m;
    double M = t_data->M;
    for(int i = 0; i < num_of_points; i++) {

        double x = generate_double(a, b, xsubi); 
        double y = generate_double(m, M, xsubi); 
        if (y < f(x)) 
            points_count++; 
    }

    *(t_data->points_count) = points_count;
    //DEBUG_PRINTF("TID: %ld\na = %lf, b = %lf, m = %lf, M = %lf\npoints_count = %d\n", (long int)t_data->tid, a, b, m, M, points_count); 

    pthread_exit(NULL);
    //return NULL;
}

double f(double x) {

    return atan(434 * x + cosh(x)) * sin(x);
}
/*double f(double x) {

    return x;
}*/

void append_to_csv(const char* filename, int n_threads, double time_taken) {
    
    int fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("open");
        return;
    }

    char buffer[256];
    int len = snprintf(buffer, sizeof(buffer), "%d,%.6f\n", n_threads, time_taken);

    if (write(fd, buffer, len) == -1) {
        perror("write");
    }

    close(fd);
}
