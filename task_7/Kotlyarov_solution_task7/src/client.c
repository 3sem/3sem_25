#include "client_funcs.h"
#include "Check_r_w_flags.h"

int main(int argc, char* argv[]) {
    if (!file_exists(CLIENT_SERVER_FIFO)) {
        DEBUG_PRINTF("ERROR: server is not launced\n");
        return -1;
    }

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGSEGV, signal_handler);

    file_types files;
    init_file_types(&files);
    
    bool parse_success = check_r_w_flags(OPTIONAL_CHECK, argv, argc, &files);
    
    if (!parse_success) {
        fprintf(stderr, "Error parsing arguments: %s\n", files.error_message);
        print_usage(argv[0]);
        cleanup_file_types(&files);
        return 1;
    }

    if (files.read_files_count == 0 && files.write_files_count == 0) {
        if (argc > 1) {
            for (int i = 1; i < argc; i++) {
                if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
                    print_usage(argv[0]);
                    cleanup_file_types(&files);
                    return 0;
                }
            }
        }
        
        printf("No files specified. Use -h for help.\n");
        cleanup_file_types(&files);
        return 0;
    }

    FILE* output_file = NULL;
    if (files.check_status & CHECK_W) {
        const char* output_filename = files.write_files[0];
        output_file = freopen(output_filename, "w", stdout);
        if (!output_file) {
            perror("freopen");
            cleanup_file_types(&files);
            return 1;
        }
    }

    pid_t pid = getpid();
    client_t client_info = {.client_id = (int) pid};
    const char* tx_path_string = "fifos/client_%d/tx";
    const char* rx_path_string = "fifos/client_%d/rx";
    snprintf(client_info.tx_path, sizeof(client_info.tx_path), 
             tx_path_string, client_info.client_id);
    snprintf(client_info.rx_path, sizeof(client_info.rx_path), 
             rx_path_string, client_info.client_id);

    if (setup_client_fifos(client_info.tx_path, client_info.rx_path) != 0) {
        DEBUG_PRINTF("Failed to setup client FIFOs\n");
        return 1;
    }

    int cl_s_fifo_fd = open(CLIENT_SERVER_FIFO, O_WRONLY);
    if (cl_s_fifo_fd == -1) {
        DEBUG_PRINTF("ERROR: '%s' was not opened\n", CLIENT_SERVER_FIFO);
        cleanup_client_fifos(client_info.tx_path, client_info.rx_path);
        return 1;  
    }

    if (register_client(&client_info, cl_s_fifo_fd) != 0) {
        DEBUG_PRINTF("ERROR: client registration failed\n");
        if (output_file) fclose(output_file);
        close(cl_s_fifo_fd);
        cleanup_client_fifos(client_info.tx_path, client_info.rx_path);
        cleanup_file_types(&files);
        return 1;
    }

    close(cl_s_fifo_fd);
    
    DEBUG_PRINTF("client_info.tx_path = %s\n", client_info.tx_path);
    DEBUG_PRINTF("client_info.rx_path = %s\n", client_info.rx_path);
    
    client_info.tx_fd = open(client_info.tx_path, O_RDONLY);
    if (client_info.tx_fd == -1) {
        perror("open TX FIFO");
        if (output_file) fclose(output_file);
        cleanup_client_fifos(client_info.tx_path, client_info.rx_path);
        cleanup_file_types(&files);
        return 1;
    }

    client_info.rx_fd = open(client_info.rx_path, O_WRONLY);
    if (client_info.rx_fd == -1) {
        perror("open RX FIFO");
        if (output_file) fclose(output_file);
        close(client_info.tx_fd);
        cleanup_client_fifos(client_info.tx_path, client_info.rx_path);
        cleanup_file_types(&files);
        return 1;
    }

    int success_count = 0;
    int total_files = files.read_files_count;
    DEBUG_PRINTF("total_files = %d\n", total_files); 
    
    if (total_files > 0) {
        for (int i = 0; i < total_files; i++) {
            const char* filename = files.read_files[i];
            if (send_get_command(&client_info, filename) != 0) {
                DEBUG_PRINTF("ERROR: Failed to send GET command for: %s\n", filename);
                continue;
            }
            if (client_recieve_and_print(client_info.tx_fd) != 0) {
                DEBUG_PRINTF("ERROR: Failed to receive file: %s\n", filename);
            } else {
                success_count++;
            }
        }
        
        if (success_count == 0) {
            DEBUG_PRINTF("No files were received successfully\n");
        }
    } else {
        printf("No files to request. Use -r <filename> to request files.\n");
        printf("Use -h for help.\n");
    }

    close(client_info.tx_fd);
    close(client_info.rx_fd);
    cleanup_client_fifos(client_info.tx_path, client_info.rx_path);
    cleanup_file_types(&files);
    if (output_file) {
        fclose(output_file);
    }

    return 0;
}
