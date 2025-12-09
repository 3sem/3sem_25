#include "server_funcs.h"

int main() {
    setup_server_signal_handlers();
    
    if (!initialize_server_fifo()) {
        return -1;
    }

    tx_threads_pool_t* pool = initialize_thread_pool();
    if (!pool) {
        cleanup_server_resources(-1, -1, NULL);
        return 1;
    }
    
    int cl_s_fifo_fd = open_command_fifo();
    if (cl_s_fifo_fd == -1) {
        cleanup_server_resources(-1, -1, pool);
        return -1;
    }
    
    int epoll_fd = setup_epoll(cl_s_fifo_fd);
    if (epoll_fd == -1) {
        cleanup_server_resources(cl_s_fifo_fd, -1, pool);
        return -1;
    }
    
    DEBUG_PRINTF("Server started, waiting for connections...\n");
    DEBUG_PRINTF("Press Ctrl+C to shutdown server gracefully\n");
    
    run_server_loop(pool, epoll_fd, cl_s_fifo_fd);

    cleanup_server_resources(cl_s_fifo_fd, epoll_fd, pool);
    DEBUG_PRINTF("Server shutdown completed\n");
    
    return 0;
}
