#include <stdio.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include "ShmSysV.hpp"
#include "Utils.hpp"

int init_semaphore();
void sem_lock(int semid, short unsigned int semnum);
void sem_unlock(int semid, short unsigned int semnum); 

int main() {
	struct timespec start, end;
	double transmission_time = 0;

	key_t key = 0;
	if ((key = ftok("/tmp", 'A')) == -1) {
	 	perror("ftok");
		exit(EXIT_FAILURE);
	}

	int shmid = 0;
	if ((shmid = shmget(key, sizeof(SharedMemory), SHM_PERMISSIONS | IPC_CREAT)) == -1) {
	 	perror("shmget");
		exit(EXIT_FAILURE);
	}

	pid_t pid = fork();
	if (pid < 0) {
		perror("fork");
		exit(EXIT_FAILURE);
	}
	if (pid == 0) {
	 	int new_file = open(FILE_NEW FILE_NAME, O_CREAT | O_WRONLY | O_TRUNC, 0644);
		if (new_file < 0) {
		    perror("opening new file failed");
		    exit(EXIT_FAILURE);
		}
                
		SharedMemory* shm_segment = (SharedMemory*)shmat(shmid, NULL, SHM_RDONLY);
		if (shm_segment == (SharedMemory*)(-1)) {
		 	perror("shmat");
			exit(EXIT_FAILURE);
		}

		int semid = semget(ftok("/tmp", 'B'), 1, 0644);
		if (semid == -1) {
			perror("semget");
			exit(EXIT_FAILURE);
		}

		printf("*Child process starts reading from shm_segment and writing to %s*\n", FILE_NEW FILE_NAME);

		ssize_t overall_bytes_wr = 0;
		while (1) {
			sem_lock(semid, 1);
			
			ssize_t write_size = write(new_file, shm_segment->buf, size_t(shm_segment->buf_size));
			if (write_size == -1) {
				perror("write");
				exit(EXIT_FAILURE);
			}

			overall_bytes_wr += write_size;

			if (shm_segment->eof == 1) {
				sem_unlock(semid, 0);
				break;
			}

			sem_unlock(semid, 0); 
		}	

		shmdt(shm_segment);
		close(new_file);

		printf("=== Child process finished reading shm_segment and writing to file ===\n");
		printf("Overall written-read bytes: %ld\n", overall_bytes_wr);

		exit(0);
	}
	else {
		int file = open(FILE_NAME, O_RDONLY | O_DIRECT, 0644);
		if (file < 0) {
		  	if (errno == ENOENT) {
				printf("=== Creating file ===\n");
		        system(FILE_CREATE);
				file = open(FILE_NAME, O_RDONLY | 0644);
			}
		   	else {
				perror("opening file failed");
				exit(EXIT_FAILURE);
			}
		}

		SharedMemory* shm_segment = (SharedMemory*)shmat(shmid, NULL, 0);
		if (shm_segment == (SharedMemory*)(-1)) {
		 	perror("shmat");
			exit(EXIT_FAILURE);
		}

		int semid = init_semaphore();

		printf("*Parent process starts reading from file %s and writing to shm_segment*\n", FILE_NAME);
		
		if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
			perror("clock_gettime");
			exit(EXIT_FAILURE);
		}

		ssize_t overall_bytes_rw = 0;
		while (1) {
			sem_lock(semid, 0);
			
			ssize_t read_size = read(file, shm_segment->buf, sizeof(shm_segment->buf));
			if (read_size == -1) {
				perror("read");
				exit(EXIT_FAILURE);
			}
			
			if (read_size == 0) {
				shm_segment->eof = 1;
				shm_segment->buf_size = 0;
				sem_unlock(semid, 1);
				break;
			}
			
			overall_bytes_rw += read_size;
			shm_segment->buf_size = (size_t)read_size;
			sem_unlock(semid, 1);
		}		

		wait(NULL);

		if (clock_gettime(CLOCK_MONOTONIC, &end) == -1) {
			perror("clock_gettime");
			exit(EXIT_FAILURE);
		}

		close(file);
		shmdt(shm_segment);
		semctl(semid, 0, IPC_RMID);

		printf("=== Parent process finished reading and writing to shm_segment ===\n");
		printf("Overall read-written bytes: %ld\n", overall_bytes_rw);
	}

	transmission_time = (double)(end.tv_sec - start.tv_sec) + (double)(end.tv_nsec - start.tv_nsec) / (double)BILLION;
	printf("=== Time ===\nOverall transmission time: %lf seconds\n", transmission_time);
	
	shmctl(shmid, IPC_RMID, NULL);

	exit(0);
}

int init_semaphore() {
 	int semid = semget(ftok("/tmp", 'B'), 2, IPC_CREAT | 0644);
	if (semid == -1) {
		perror("semget");
		exit(EXIT_FAILURE);
	}

	union semun {
	 	int val;
		struct semid_ds* buf;
		unsigned short* array;
	} arg;

	arg.val = 1;
	if (semctl(semid, 0, SETVAL, arg) == -1) {
		perror("semctl SETVAL");
		exit(EXIT_FAILURE);
	}

	return semid;
}

void sem_lock(int semid, short unsigned int semnum) {
 	struct sembuf sb = {semnum, -1, 0};
	if (semop(semid, &sb, 1) == -1) {
		perror("semop lock");
		exit(EXIT_FAILURE);
	}
}

void sem_unlock(int semid, short unsigned int semnum) {
 	struct sembuf sb = {semnum, 1, 0};
	if (semop(semid, &sb, 1) == -1) {
		perror("semop unlock");
		exit(EXIT_FAILURE);
	}
}