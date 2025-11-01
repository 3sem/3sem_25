#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <time.h>
#include <errno.h>
#include <sys/wait.h>

#define NUM_POINTS  1e9
#define NUM_THREADS 1
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

		pthread_t threads[NUM_THREADS] = {};
		thread_data_t thread_data[NUM_THREADS] = {};
		
		for (int i = 0; i < NUM_THREADS; i++) {
			double cur_a = a + (b - a) / NUM_THREADS * i;
			double cur_b = a + (b - a) / NUM_THREADS * (i + 1);
			double cur_A = f(cur_a);
			double cur_B = f(cur_b);
			thread_data[i] = {.thread_id = i, .points_per_thread = NUM_POINTS, .func = f, .a = cur_a, 
					  .b = cur_b, .y_max = (cur_A > cur_B ? cur_A : cur_B), .results = shared_results};
			pthread_create(&threads[i], NULL, thread_calculate, (void*)(&thread_data[i]));
		}
			
		for (int i = 0; i < NUM_THREADS; i++)
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
		for (int i = 0; i < NUM_THREADS; i++) 
		 	total += shared_results[i];
		printf("Answer: %.6f\n", total);

		shmdt(shared_results);
	}

	shmctl(shmid, IPC_RMID, 0);
	
        transmission_time = (double)(end.tv_sec - start.tv_sec) + (double)(end.tv_nsec - start.tv_nsec) / (double)BILLION;
	printf("=== Time ===\nOverall transmission time: %lf seconds\n", transmission_time);
	
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

	printf("Thread %d ends calculating: %d of %d points are under the curve\n",
		thread_data->thread_id, points_count, thread_data->points_per_thread);

	pthread_exit(NULL);
}

double f(double x) {
 	return x * x;
}
