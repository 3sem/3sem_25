#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <cstdlib>
#include <string.h>
#include <wait.h>
#include "MsgSysV.hpp" 

int main() {
	
	struct message msg;
	msg.mtype = 1;

	const char text[] = "This message was sended from parent to child with message queue!\n";
	strcpy(msg.mtext, text);

	msg.mtext[MESSAGE_TEXT_SIZE - 1] = '\0';

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
		    return EXIT_FAILURE;
		}

		while ((msgrcv(msgid, &msg, sizeof(msg.mtext), 1, 0) > -1)) {
		 	//printf("Received message with type: %ld - %s", msg.mtype, msg.mtext);
			if (write(new_file, msg.mtext, sizeof(msg.mtext)) == -1) {
				perror("write");
				return EXIT_FAILURE;
			}
		}

		close(new_file);

		if ((msgctl(msgid, IPC_RMID, NULL) == -1))
	 		perror("msgctl");
	}
	else {
		printf("Hello from parent!\n");
		
		int file = open(FILE_NAME, O_RDONLY | 0644);
		if (file < 0) {
		  	if (errno == ENOENT)
		        	system(FILE_CREATE);
				file = open(FILE_NAME, O_RDONLY | 0644);
		   	else {
		        	perror("opening file failed");
		        	return EXIT_FAILURE;
		    	}
		}

		while (read(file, msg.mtext, sizeof(msg.text)) > -1) {
			if (msgsnd(msgid, &msg, sizeof(msg.mtext), 0) == -1) {
				perror("msgsnd");
				exit(EXIT_FAILURE);
			}
		}
		wait(NULL);
		close(file);				
	}

 	return 0;
}                   