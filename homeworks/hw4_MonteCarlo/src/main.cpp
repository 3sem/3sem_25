#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <time.h>
#include <errno.h>
#include <sys/wait.h>
#include <math.h>

#define NUM_POINTS  1e9
#define NUM_THREADS 50
#define BILLION 1e9

typedef double(*func_t)(double); 

struct thread_data_t {
	int thread_id;
	int points_per_thread;
  	func_t func;
	double a;
	double b;
	double y_max;
	int shm_id;
	double* results;
};

void *thread_calculate(void* args);
double f(double x);

int main() {
	struct timespec start, end;
	double transmission_time = 0;

	key_t key = 0;
	if ((key = ftok("/tmp", 'A')) == -1) {
	 	perror("ftok");
		exit(EXIT_FAILURE);
	}

	int shmid = 0;
	if ((shmid = shmget(key, NUM_THREADS * sizeof(double), 0666 | IPC_CREAT)) == -1) {
	 	perror("shmget");
		exit(EXIT_FAILURE);
	} 

	double a, b;
	printf("Enter integration parameters:\n");
	printf("A: ");
	scanf("%lg", &a);
	printf("B: ");
	scanf("%lg", &b);
	
	for (int num_threads = 1; num_threads < NUM_THREADS; num_threads++) {

		pid_t pid = fork();	
		if (pid < 0) {
	 		perror("fork");
			exit(EXIT_FAILURE);
		}
		if (pid == 0) {
			double* shared_results = (double*)shmat(shmid, NULL, 0);
			if (shared_results == (void*)-1) {
		 		perror("shmat");
				exit(EXIT_FAILURE);
			}

			pthread_t* threads = (pthread_t*)calloc((size_t)num_threads, sizeof(pthread_t));
			thread_data_t* thread_data = (thread_data_t*)calloc((size_t)num_threads, sizeof(thread_data_t));
		
			for (int i = 0; i < num_threads; i++) {
				double cur_a = a + (b - a) / num_threads * i;
				double cur_b = a + (b - a) / num_threads * (i + 1);
				double cur_A = f(cur_a);
				double cur_B = f(cur_b);
				thread_data[i] = {.thread_id = i, .points_per_thread = (int)(NUM_POINTS / num_threads), .func = f, .a = cur_a, 
						  .b = cur_b, .y_max = (cur_A > cur_B ? cur_A : cur_B), .results = shared_results};
				pthread_create(&threads[i], NULL, thread_calculate, (void*)(&thread_data[i]));
			}
			
			for (int i = 0; i < num_threads; i++)
				pthread_join(threads[i], NULL);
		
			shmdt(shared_results);
			exit(EXIT_SUCCESS);
		}
		else {
			double* shared_results = (double*)shmat(shmid, NULL, 0);
			if (shared_results == (void*)-1) {
			 	perror("shmat");
				exit(EXIT_FAILURE);
			}

	 		if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
				perror("clock_gettime");
				exit(EXIT_FAILURE);
			}		

			wait(NULL);

			if (clock_gettime(CLOCK_MONOTONIC, &end) == -1) {
				perror("clock_gettime");
				exit(EXIT_FAILURE);
			}

			double total = 0;
			for (int i = 0; i < num_threads; i++) 
		 		total += shared_results[i];
			printf("Answer: %.6f\n", total);

			shmdt(shared_results);
		}

	
        	transmission_time = (double)(end.tv_sec - start.tv_sec) + (double)(end.tv_nsec - start.tv_nsec) / (double)BILLION;
		printf("=== Time ===\nNumber of threads: %d - Overall transmission time: %lf seconds\n", num_threads, transmission_time);
	
	}
	
	shmctl(shmid, IPC_RMID, 0);

	exit(EXIT_SUCCESS);
}

void *thread_calculate(void *args) {
	if (!args) {
		perror("Null pointer");
		exit(EXIT_FAILURE);
	}	

	thread_data_t* thread_data = (thread_data_t*)args;
	int points_count = 0;

	unsigned int seed = (unsigned int)(time(NULL) ^ thread_data->thread_id);

	for (int i = 0; i < thread_data->points_per_thread; i++) {
	 	double x = thread_data->a + ((double)rand_r(&seed) / RAND_MAX) * (thread_data->b - thread_data->a);
		double y = ((double)rand_r(&seed) / RAND_MAX) * thread_data->y_max;

		if (fabs(y) - fabs(thread_data->func(x)) <= 0)
			points_count++;
	}

	thread_data->results[thread_data->thread_id] = (double)points_count / thread_data->points_per_thread * 
							(thread_data->b - thread_data->a) * thread_data->y_max;

	pthread_exit(NULL);
}

double f(double x) {
 	return x * x;
}
