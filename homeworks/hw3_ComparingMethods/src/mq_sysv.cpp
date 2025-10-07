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
#include "MsgSysV.hpp" 

int main() {
	
	struct message msg;
	msg.mtype = 1;

	key_t key = 0;
	if ((key = ftok("/tmp", 'A')) == -1) {
	 	perror("ftok");
		exit(EXIT_FAILURE);
	}

	int msgid = 0;
	if ((msgid = msgget(key, QUEUE_PERMISSIONS | IPC_CREAT)) == -1) {
		perror("msgget");
		exit(EXIT_FAILURE);
	}

	pid_t pid = fork();
	if (pid == -1) {
		perror("fork");
		exit(EXIT_FAILURE);
	}	

	if (pid == 0) {
		printf("Hello from child!\n");

		int new_file = open(FILE_NEW FILE_NAME, O_CREAT | O_WRONLY | O_TRUNC, 0644);
		if (new_file < 0) {
		    perror("opening new file failed");
		    exit(EXIT_FAILURE);
		}

		while (1) {
			ssize_t rcv_size = msgrcv(msgid, &msg, sizeof(msg.mtext), 1, 0);
			if (rcv_size < 0) {
				perror("msgrcv");
				exit(EXIT_FAILURE);
			}
			else if (!memcmp(msg.mtext, MSG_END, sizeof(MSG_END))) {
				break;
			}
			else {
				if (write(new_file, msg.mtext, (size_t)rcv_size) == -1) {
					perror("write");
					exit(EXIT_FAILURE);
				}
			}
		}

		close(new_file);

		if ((msgctl(msgid, IPC_RMID, NULL) == -1)) {
	 		perror("msgctl");
			exit(EXIT_FAILURE);
		}
	}
	else {
		printf("Hello from parent!\n");
		
		int file = open(FILE_NAME, O_RDONLY | 0644);
		if (file < 0) {
		  	if (errno == ENOENT) {
		        	system(FILE_CREATE);
					file = open(FILE_NAME, O_RDONLY | 0644);
			}
		   	else {
		        	perror("opening file failed");
		        	exit(EXIT_FAILURE);
		    	}
		}

		ssize_t read_size = 0;
		while ((read_size = read(file, msg.mtext, sizeof(msg.mtext))) > 0) {
			if (msgsnd(msgid, &msg, (size_t)read_size, 0) == -1) {
				perror("msgsnd");
				exit(EXIT_FAILURE);
			}
		}

		memcpy(msg.mtext, MSG_END, sizeof(MSG_END));
		if (msgsnd(msgid, &msg, sizeof(MSG_END), 0) == -1) {
			perror("msgsnd");
			exit(EXIT_FAILURE);
		}

		wait(NULL);
		close(file);				
	}

 	exit(0);
}                   