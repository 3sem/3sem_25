#include <stdio.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "Fifo.hpp"

int main() {
	struct timespec start, end;
	double transmission_time = 0;

	struct Fifo fifo_struct = {};

	key_t key = 0;
	if ((key = ftok("/tmp", 'A')) == -1) {
	 	perror("ftok");
		exit(EXIT_FAILURE);
	}

	if (mknod(FIFO_NAME, S_IFIFO | FIFO_PERMISSIONS, 0) == -1) {
		perror("mknod");
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

		int fifo_file = open(FIFO_NAME, O_RDONLY);
		if (fifo_file < 0) {
			perror("opening fifo failed");
			exit(EXIT_FAILURE);
		}
                
		printf("*Child process starts reading fifo and writting to %s*\n", FILE_NEW FILE_NAME);

		ssize_t read_size = 0;
		ssize_t overall_bytes_read = 0, overall_bytes_written = 0;
		while ((read_size = read(fifo_file, fifo_struct.buf, FIFO_SIZE)) > 0) {
			overall_bytes_read += read_size;	
			
			ssize_t write_size = 0;
			while (write_size < read_size) {
				ssize_t curr_write_size = write(new_file, fifo_struct.buf + write_size, (size_t)(read_size - write_size));
				if (curr_write_size == -1) {
					perror("write");
					exit(EXIT_FAILURE);
				}
				write_size += curr_write_size;
			}
			overall_bytes_written += write_size;
		}
		
		close(fifo_file);
		close(new_file);

		printf("=== Child process finished reading fifo and writting to file ===\n");
		printf("Overall read bytes: %ld\n", overall_bytes_read);
		printf("Overall written bytes: %ld\n", overall_bytes_written);

		exit(0);
	}
	else {
		int file = open(FILE_NAME, O_RDONLY | 0644);
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

		int fifo_file = open(FIFO_NAME, O_WRONLY);
		if (fifo_file < 0) {
			perror("opening fifo failed");
			exit(EXIT_FAILURE);
		}

		printf("*Parent process starts reading from file %s and writing to fifo*\n", FILE_NAME);
		
		if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
			perror("clock_gettime");
			exit(EXIT_FAILURE);
		}

		ssize_t read_size = 0;
		ssize_t overall_bytes_read = 0, overall_bytes_written = 0;
		while ((read_size = read(file, fifo_struct.buf, FIFO_SIZE)) > 0) {
			overall_bytes_read += read_size;	
			
			ssize_t write_size = 0;
			while (write_size < read_size) {
				ssize_t curr_write_size = write(fifo_file, fifo_struct.buf + write_size, (size_t)(read_size - write_size));
				if (curr_write_size == -1) {
					perror("write");
					exit(EXIT_FAILURE);
				}
				write_size += curr_write_size;
			}
			overall_bytes_written += write_size;
		}			
		
		close(fifo_file);
		wait(NULL);

		if (clock_gettime(CLOCK_MONOTONIC, &end) == -1) {
			perror("clock_gettime");
			exit(EXIT_FAILURE);
		}

		close(file);
		
		printf("=== Parent process finished reading file and writing to fifo ===\n");
		printf("Overall read bytes: %ld\n", overall_bytes_read);
		printf("Overall written bytes: %ld\n", overall_bytes_written);
	}

	transmission_time = (end.tv_sec - start.tv_sec) + (double)(end.tv_nsec - start.tv_nsec) / (double)BILLION;
	printf("=== Time ===\nOverall transmission time: %lf seconds\n", transmission_time);
	
	unlink(FIFO_NAME);
	exit(0);
}