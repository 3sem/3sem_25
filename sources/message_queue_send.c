#include <sys/types.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <memory.h>
#include <errno.h>

#include "message_queue_send.h"
#include "common.h"

int recieve_mqueue (Mqueue *mqueue, int fd_in);
int send_mqueue    (Mqueue *mqueue, int fd_out);

Mqueue* ctor_mqueue() {
    Mqueue *mqueue;

    CALLOC_SELF(mqueue);

    int msgflg = IPC_CREAT | 0666;
    mqueue->msgkey = ftok(".", 'P');

    if ((mqueue->msqid = msgget(mqueue->msgkey, msgflg)) < 0) {
        perror("error msgget ctor\n");
        dtor_mqueue(mqueue);
        return 0;
    }

    return mqueue;
}

int dtor_mqueue(Mqueue *mqueue) {
    if (!mqueue) return 0;

    SET_ZERO(mqueue);
    free(mqueue);

    return 0;
}

int send_file_mqueue (Mqueue *mqueue, const char *src_file, const char *dst_file) {
    if (!mqueue) {
        perror("no mqueue object\n");
        return -1;
    }

    if (!src_file || !dst_file) {
        perror("broken names\n");
        return -1;
    }

    int fd_dst, fd_src;

    if ((fd_src = open(src_file, O_RDONLY | O_CREAT, 0666)) == -1) {
        perror("open source file failed\n");
        return -1;
    }

    if ((fd_dst = open(dst_file, O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1) {
        perror("open destination file failed\n");
        close(fd_src);
        return -1;
    }

    pid_t pid1 = fork();

    if (pid1 == -1) {
        close(fd_src);
        close(fd_dst);
        perror("fork recieve failed (mqueue)\n");
        return -1;

    } else if (pid1 == 0) {
        close(fd_src);
        int result = send_mqueue(mqueue, fd_dst);
        dtor_mqueue(mqueue);
        close(fd_dst);

        if (result == 0) exit(0);
        perror("send failed (mqueue)\n");
        exit(-1);
    }
    pid_t pid2 = fork();
    if (pid2 == -1) {
        close(fd_src);
        close(fd_dst);
        kill(pid1, SIGINT);
        perror("fork send failed (mqueue)\n");
        return -1;

    } else if (pid2 == 0) {
        close(fd_dst);
        int result = recieve_mqueue(mqueue, fd_src);
        dtor_mqueue(mqueue);
        close(fd_src);

        if (result == 0) exit(0);
        perror("recieve failed (mqueue)\n");
        exit(-1);
    }
    close(fd_dst);
    close(fd_src);
    waitpid(pid2, NULL, 0);
    waitpid(pid1, NULL, 0);
    return 0;
}

int recieve_mqueue (Mqueue *mqueue, int fd_in) {
    SET_BLOCK(fd_in);
    do {
        if ((mqueue->mbuf.size = read(fd_in, mqueue->mbuf.data, MSG_SIZE)) == -1) {
            printf("\x1b[33m""ERROR in read; errno = %d\n""\x1b[0m", errno);
            return -1;
        }
        msgsnd(mqueue->msqid, &mqueue->mbuf, sizeof(mqueue->mbuf.size) + sizeof(mqueue->mbuf.data), 0);
        printf("sended %d bytes\n", mqueue->mbuf.size);
    } while (mqueue->mbuf.size != 0);

    printf("recieve ended\n");

    return 0;
}
int send_mqueue (Mqueue *mqueue, int fd_out) {
    SET_BLOCK(fd_out);
    do {
        printf("STARTED\n");
        msgrcv(mqueue->msqid, &mqueue->mbuf, sizeof(mqueue->mbuf.size) + sizeof(mqueue->mbuf.data), 1, 0);
        printf("recieved %d bytes\n", mqueue->mbuf.size);
        if ((mqueue->mbuf.size = write(fd_out, mqueue->mbuf.data, MSG_SIZE)) == -1) {
            printf("\x1b[33m""ERROR in write; errno = %d\n""\x1b[0m", errno);
            return -1;
        }
    } while (mqueue->mbuf.size != 0);

    printf("send ended\n");

    return 0;
}
