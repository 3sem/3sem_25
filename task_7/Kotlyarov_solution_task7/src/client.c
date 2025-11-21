#include "client_funcs.h"
#include "Check_r_w_flags.h"

int main(int argc, char* argv[]) {
    if (!file_exists(CLIENT_SERVER_FIFO)) {
        DEBUG_PRINTF("ERROR: server is not launced\n");
        return -1;
    }

    file_types files = {};
    Check_r_w_flags(OPTIONAL_CHECK, argv, argc, &files)
    
    if (files.check_status & CHECK_R) {
        FILE* input_file = freopen(files.read_files[0], "r", stdin);
        if (!input_file) {
            perror("freopen");
            return 1;
        }
    }

    if (files.check_status & CHECK_W) {
        FILE* output_file = freopen(files.write_files[0], "w", stdout);
        if (!output_file) {
            perror("freopen");
            return 1;
        }
    }

    int cl_s_fifo_fd = open(CLIENT_SERVER_FIFO, O_WRONLY);
    if (cl_s_fifo_fd == -1) {
        DEBUG_PRINTF("ERROR: CLIENT_SERVER_FIFO was not opened\n");
        return 1;
    }

    pid_t pid = getpid();
    client_t client_info = {.client_number = (int) pid};
    const char* tx_path_string = "fifos/client_%s/tx";
    const char* rx_path_string = "fifos/client_%s/rx";
    snprintf(client_info.tx_path, sizeof(client_info.tx_path), 
             tx_path_string, client_info.client_number);
    snprintf(client_info.rx_path, sizeof(client_info.rx_path), 
             rx_path_string, client_info.client_number);

    if (register_client(&client_info, cl_s_fifo_fd) != 0) {
        DEBUG_PRINTF("ERROR: client %d registration failed\n", client_info.client_number);
        return 1;
    }

}
