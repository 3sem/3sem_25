#include "client_funcs.h"

int register_client(client_t* client_info, int cl_s_fifo_fd) {

    char reg_cmd[256] = {}; 
    const char* reg_cmd_string = "REGISTER %s %s";
    snprintf(reg_cmd, sizeof(reg_cmd), reg_cmd_string, 
             client_info.tx_path, client_info.rx_path);

    if (write(cl_s_fifo_fd, reg_cmd, strlen(reg_cmd)) <= 0) {
        DEBUG_PRINTF("ERROR: write failed\n");
        return 1;
    }

    if (setup_client_fifos(&client_info) != 0) {
        DEBUG_PRINTF("Failed to setup client FIFOs\n");
        return 1;
    }


}
