#include "server_funcs.h"

tx_threads_pool_t* thread_pool;

int main() {
    if (create_fifo(CLIENT_SERVER_FIFO) != 0) {
        return -1;
    }

    thread_pool = tx_pool_init(MIN_THREADS);
    if (!thread_pool) {
        DEBUG_PRINTF("Failed to initialize thread pool\n");
        return 1;
    }
    
    int cl_s_fifo_fd = open(CLIENT_SERVER_FIFO, O_RDWR | O_NONBLOCK);
    if (cl_s_fifo_fd == -1) {
        perror("open CLIENT_SERVER_FIFO");
        return -1;
    }
    
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        return -1;
    }
    
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = cl_s_fifo_fd;
    
    event.data.u32 = (unsigned int)FD_TYPE_CMD_FIFO;
    
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cl_s_fifo_fd, &event) == -1) {
        perror("epoll_ctl");
        return -1;
    }
    
    DEBUG_PRINTF("Server started, waiting for connections...\n");
    
    run_server_loop(epoll_fd, thread_pool);

    tx_pool_destroy(thread_pool);
    close(epoll_fd);
    close(cl_s_fifo_fd);
    return 0;
}
