#include "server_funcs.h"

int main() {
    setup_server_signal_handlers();
    
    if (create_fifo(CLIENT_SERVER_FIFO) != 0) {
        return -1;
    }

    tx_threads_pool_t* pool = tx_pool_init(MIN_THREADS);
    if (!pool) {
        DEBUG_PRINTF("Failed to initialize thread pool\n");
        cleanup_server_resources(-1, -1, NULL);
        return 1;
    }
    
    int cl_s_fifo_fd = open(CLIENT_SERVER_FIFO, O_RDWR | O_NONBLOCK);
    if (cl_s_fifo_fd == -1) {
        perror("open CLIENT_SERVER_FIFO");
        cleanup_server_resources(-1, -1, pool);
        return -1;
    }
    
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        cleanup_server_resources(cl_s_fifo_fd, -1, pool);
        return -1;
    }
    
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = cl_s_fifo_fd;
    event.data.u32 = FD_TYPE_CMD_FIFO;
    
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cl_s_fifo_fd, &event) == -1) {
        perror("epoll_ctl");
        cleanup_server_resources(cl_s_fifo_fd, epoll_fd, pool);
        return -1;
    }
    
    DEBUG_PRINTF("Server started, waiting for connections...\n");
    DEBUG_PRINTF("Press Ctrl+C to shutdown server gracefully\n");
    
    run_server_loop(pool, epoll_fd, cl_s_fifo_fd);

    cleanup_server_resources(cl_s_fifo_fd, epoll_fd, pool);
    DEBUG_PRINTF("Server shutdown completed\n");
    
    return 0;
}
