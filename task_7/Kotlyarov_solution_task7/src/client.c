#include "client_funcs.h"

int main(int argc, char* argv[]) {
    if (!file_exists(CLIENT_SERVER_FIFO)) {
        DEBUG_PRINTF("ERROR: server is not launched\n");
        return -1;
    }

    setup_client_signals();

    file_types files;
    if (!parse_client_arguments(argc, argv, &files)) {
        return 1;
    }

    FILE* output_file = setup_output_redirection(&files);
    if (output_file == NULL && (files.check_status & CHECK_W)) {
        return 1;
    }

    client_t client_info;
    if (!initialize_client(&client_info)) {
        cleanup_file_types(&files);
        if (output_file) fclose(output_file);
        return 1;
    }

    if (!register_with_server(&client_info)) {
        cleanup_client_resources(&client_info, &files, output_file);
        return 1;
    }

    process_file_requests(&client_info, &files);

    DEBUG_PRINTF("Sending DISCONNECT command to server\n");
    send_disconnect_command(&client_info);

    cleanup_client_resources(&client_info, &files, output_file);
    return 0;
}
