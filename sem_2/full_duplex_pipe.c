#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pthread.h>
#include <fcntl.h>
#include <semaphore.h>
#include <time.h>

#define BUFFER_SIZE (1*1024)

typedef struct pPipe Pipe;

typedef struct op_table Ops;

typedef struct op_table {
    size_t (*rcv)(Pipe *self, char dir);
    size_t (*snd)(Pipe *self, char dir, size_t bytes);
} Ops;

typedef struct pPipe {
        char* data;
        int fd_direct[2]; // parent-->child direction
        int fd_back[2]; // child-->parent direction
        size_t len;
        Ops actions;
} Pipe;

size_t sendData(Pipe *self, char dir, size_t bytes) {
    if (dir) // from parent to child
        return write(self->fd_direct[1], self->data, bytes);
    else // from child to parent
        return write(self->fd_back[1], self->data, bytes);
}

size_t receiveData(Pipe *self, char dir) {
    if (dir) // from parent to child
        return read(self->fd_direct[0], self->data, self->len);
    else // from child to parent
        return read(self->fd_back[0], self->data, self->len);
}

Pipe *pipeInit(int n) {
    Pipe *p = (Pipe *) malloc(sizeof(Pipe));
    if (!p) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }

    p->len = n;
    p->data = (char *) calloc(n, sizeof(char));
    if (!p->data) {
        fprintf(stderr, "Memory allocation failed\n");
        free(p);
        return NULL;
    }

    if ((pipe(p->fd_direct) < 0) || (pipe(p->fd_back) < 0)) {
        fprintf(stderr, "Failed to initialize pipe\n");
        free(p->data);
        free(p);
        return NULL;
    }

    p->actions.rcv = receiveData;
    p->actions.snd = sendData;

    return p;
}

sem_t main_semaphore, child_semaphore;
volatile int running = 1;

void *thread_func(void *arg) {
    Pipe *p = (Pipe *) arg;
    while (running) {
        sem_wait(&child_semaphore);
        if (!running)
            break;
        ssize_t bytes_read = p->actions.rcv(p, 1);
        if (bytes_read > 0)
            p->actions.snd(p, 0, bytes_read);
        sem_post(&main_semaphore);
    }
}

int main() {
    Pipe *p = pipeInit(BUFFER_SIZE);
    sem_init(&main_semaphore, 0, 1);
    sem_init(&child_semaphore, 0, 0);

    char *input_path = calloc(64, sizeof(char));
    char *output_path = calloc(64, sizeof(char));
    if (!input_path || !output_path) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    scanf("%s", input_path);
    scanf("%s", output_path);

    int input_file = open(input_path, O_RDONLY);
    if (input_file < 0) {
        fprintf(stderr, "Failed to open input file\n");
        return 1;
    }

    int output_file = open(output_path, O_WRONLY);
    if (output_file < 0) {
        fprintf(stderr, "Failed to open output file\n");
        close(input_file);
        return 1;
    }

    int bytes_written = 0;

    pthread_t thread;
    if (pthread_create(&thread, NULL, thread_func, (void *)p) != 0) {
        fprintf(stderr, "Failed to create thread\n");
        return 1;
    }

    clock_t start = clock();

    while (1) {
        int bytes_read = read(input_file, p->data, BUFFER_SIZE);
        if (bytes_read < 0) {
            fprintf(stderr, "Error while reading from file\n");
            break;
        } else if (bytes_read == 0) {
            break;
        } else {
            p->actions.snd(p, 1, bytes_read);
        }
        sem_post(&child_semaphore);

        sem_wait(&main_semaphore);
        bytes_read = p->actions.rcv(p, 0);
        if (bytes_read < 0) {
            fprintf(stderr, "Error while reading from pipe\n");
        } else if (bytes_read > 0) {
            write(output_file, p->data, bytes_read);
        }
    }

    clock_t end = clock();
    printf("Time taken: %03f seconds\n", ((double) (end - start)) / CLOCKS_PER_SEC);

    running = 0;
    sem_post(&child_semaphore);
    pthread_join(thread, NULL);

    sem_destroy(&main_semaphore);
    sem_destroy(&child_semaphore);

    close(p->fd_back[0]);
    close(p->fd_back[1]);
    close(p->fd_direct[0]);
    close(p->fd_direct[1]);
    close(input_file);
    close(output_file);
    free(p->data);
    free(p);
    free(input_path);
    free(output_path);

    return 0;
}
