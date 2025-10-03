#include "Monte_Carlo_integration.h"

int main(int argc, char* argv[]) {

    int num_of_threads, num_of_points;
    double a, b, M, m;
    if (argc < 7) {

        DEBUG_PRINTF("Usage: ./Monte_Carlo_integration n_threads, n_points a b m M\n");
        DEBUG_PRINTF("n_threads - number of threads, n_points - number of points, a - left end of the interval, b - right, M - maximum of function (or greater), m - minimum (or less)\n");
        DEBUG_PRINTF("Continue with default input data? (y / n): ");
        int input_char = getchar();
        if (input_char == 'y' || input_char == '\n') {

            num_of_threads = Default_num_of_threads, num_of_points = Default_num_of_points;
            a = Default_a, b = Default_b, M = Default_M, m = Default_m;
        }

        else {

            return 0;
        }
    }

    else {

        num_of_threads = atoi(argv[1]), num_of_points = atoi(argv[2]);
        a = atof(argv[3]), b = atof(argv[4]), M = atof(argv[6]), m = atof(argv[5]);
    }

    key_t shm_key = ftok(".", 'M');
    if (shm_key == -1) {
        perror("ftok (shm)");
        return 1;
    }

    int shmid = shmget(shm_key, num_of_threads * sizeof(int), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        return 1;
    }

    int* points_under_curve_counts = (int*) shmat(shmid, NULL, 0);
    if (points_under_curve_counts  == (void*) -1) {
        perror("shmat");
        return 1;
    }
    
    pid_t pid = fork();
    if (pid < 0) {

        DEBUG_PRINTF("fork failed!\n");
        return -1;
    }
    
    struct timespec start, end;
    if (pid == 0) {

        MC_thread_data* tids_data = (MC_thread_data*) calloc(num_of_threads, sizeof(MC_thread_data));
        if (!tids_data) {

            perror("calloc");
            shmdt(points_under_curve_counts);
            return 1;
        }

        DivisorsPair d_pair = find_closest_divisors(num_of_threads);
        if (d_pair.rows == 0 || d_pair.cols == 0) {

            DEBUG_PRINTF("Error: find_closest_divisors returned 0 for rows or cols\n");
            shmdt(points_under_curve_counts);
            return 1;
        }
        int thread_num_of_points = num_of_points / num_of_threads;
        double dx = (b - a) / d_pair.cols;
        double dy = (M - m) / d_pair.rows;

        for (int i = 0; i < num_of_threads; i++) {
        
            tids_data[i].a = a + (i % d_pair.cols) * dx;
            tids_data[i].b = tids_data[i].a + dx;
            tids_data[i].m = m + (i / d_pair.cols) * dy;
            tids_data[i].M = tids_data[i].m + dy;
            tids_data[i].num_of_points = thread_num_of_points;
            tids_data[i].points_count = points_under_curve_counts + i;

            pthread_create(&tids_data[i].tid, NULL, generate_and_count_points, tids_data + i);
        }

        for (int i = 0; i < num_of_threads; i++)
            pthread_join(tids_data[i].tid, NULL);

        shmdt(points_under_curve_counts);
        free(tids_data);
        exit(0);
    }

    else {
        
        clock_gettime(CLOCK_MONOTONIC, &start);
        int status;
		waitpid(pid, &status, 0);
        clock_gettime(CLOCK_MONOTONIC, &end);
        clock_gettime(CLOCK_MONOTONIC, &end);
        printf("Ret code: %d\n", WEXITSTATUS(status));
        double time_taken = (end.tv_sec - start.tv_sec) +
                            (end.tv_nsec - start.tv_nsec) / 1e9;
        int points_under_curve_total = 0;
        for (int i = 0; i < num_of_threads; i++) 
            points_under_curve_total += points_under_curve_counts[i];  

        double area_under_curve = (b - a) *  
                                  ((M - m) * 
                                  ((double) points_under_curve_total) / ((double) num_of_points) + m);

        printf("Area under curve: %lf\n", area_under_curve);
        fprintf(stderr, "%s: Time consumed = %.6f seconds\n", argv[0], time_taken);
        append_to_csv("results.csv", num_of_threads, time_taken);
    }
    
    shmdt(points_under_curve_counts);
    shmctl(shmid, IPC_RMID, NULL);
    return 0;
}

