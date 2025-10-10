#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <cstdlib>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>
#include "MsgSysV.hpp" 

int main() {
	struct timespec start, end;
	double transmission_time = 0;
	
	struct message msg;
	msg.mtype = 1;

	key_t key = 0;
	if ((key = ftok("/tmp", 'A')) == -1) {
	 	perror("ftok");
		exit(EXIT_FAILURE);
	}

	int msgid = 0;
	if ((msgid = msgget(key, MSQ_PERMISSIONS | IPC_CREAT)) == -1) {
		perror("msgget");
		exit(EXIT_FAILURE);
	}

	pid_t pid = fork();
	if (pid == -1) {
		perror("fork");
		exit(EXIT_FAILURE);
	}	

	if (pid == 0) {
		int new_file = open(FILE_NEW FILE_NAME, O_CREAT | O_WRONLY | O_TRUNC, 0644);
		if (new_file < 0) {
		    perror("opening new file failed");
		    exit(EXIT_FAILURE);
		}

		printf("*Child process starts receiving messages and writting them to %s*\n", FILE_NEW FILE_NAME);

		ssize_t overall_bytes_recieved = 0, overall_bytes_written = 0;
		while (1) {
			ssize_t rcv_size = msgrcv(msgid, &msg, sizeof(msg.mtext), 1, 0);
			if (rcv_size < 0) {
				perror("msgrcv");
				exit(EXIT_FAILURE);
			}
			else if (!memcmp(msg.mtext, MSQ_END, sizeof(MSQ_END))) {
				break;
			}
			else {
				overall_bytes_recieved += rcv_size;
				ssize_t wrt_size = write(new_file, msg.mtext, (size_t)rcv_size);
				if (wrt_size == -1) {
					perror("write");
					exit(EXIT_FAILURE);
				}
				overall_bytes_written += wrt_size;
			}
		}

		close(new_file);

		printf("=== Child process finished receiving and writting messages ===\n");
		printf("Overall recieved bytes: %ld\n", overall_bytes_recieved);
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

		printf("*Parent process starts reading from file %s and sending data*\n", FILE_NAME);

		if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
			perror("clock_gettime");
			exit(EXIT_FAILURE);
		}

		ssize_t read_size = 0;
		ssize_t overall_bytes_read = 0, overall_bytes_sended = 0;
		while ((read_size = read(file, msg.mtext, sizeof(msg.mtext))) > 0) {
			overall_bytes_read += read_size;
			if (msgsnd(msgid, &msg, (size_t)read_size, 0) == -1) {
				perror("msgsnd");
				exit(EXIT_FAILURE);
			}
			overall_bytes_sended += read_size;
		}

		memcpy(msg.mtext, MSQ_END, sizeof(MSQ_END));
		if (msgsnd(msgid, &msg, sizeof(MSQ_END), 0) == -1) {
			perror("msgsnd");
			exit(EXIT_FAILURE);
		}

		wait(NULL);

		if (clock_gettime(CLOCK_MONOTONIC, &end) == -1) {
			perror("clock_gettime");
			exit(EXIT_FAILURE);
		}

		close(file);

		printf("=== Parent process finished reading and sending messages ===\n");
		printf("Overall read bytes: %ld\n", overall_bytes_read);
		printf("Overall sended bytes: %ld\n", overall_bytes_sended);
	}

	transmission_time = (end.tv_sec - start.tv_sec) + (double)(end.tv_nsec - start.tv_nsec) / (double)BILLION;
	printf("=== Time ===\nOverall transmission time: %lf seconds\n", transmission_time);

	if ((msgctl(msgid, IPC_RMID, NULL) == -1)) {
 		perror("msgctl");
		exit(EXIT_FAILURE);
	}

 	exit(0);
}                   