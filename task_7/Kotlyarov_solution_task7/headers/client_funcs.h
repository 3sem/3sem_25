#ifndef CLIENT_FUNCS
#define CLIENT_FUNCS

#define _GNU_SOURCE

#include <signal.h>
#include <sys/select.h>
#include <sys/time.h>
#include "fifo_utils.h"
#include "client_server_cfg.h"
#include "Debug_printf.h"
#include "Check_r_w_flags.h"

#define BUFFER_SIZE (4 * 1024 * 1024)

void setup_client_signals(void);
bool parse_client_arguments(int argc, char* argv[], file_types* files);
FILE* setup_output_redirection(file_types* files);
bool initialize_client(client_t* client_info);
bool register_with_server(client_t* client_info);
void process_file_requests(client_t* client_info, file_types* files);
void cleanup_client_resources(client_t* client_info, file_types* files, FILE* output_file);
void signal_handler(int sig);
int setup_client_fifos(const char* tx_path, const char* rx_path);
void cleanup_client_fifos(const char* tx_path, const char* rx_path);
int register_client(client_t* client_info, int cl_s_fifo_fd);
int client_recieve_and_print(int tx_fd);
int send_get_command(client_t* client_info, const char* filename);
int send_disconnect_command(client_t* client_info);

#endif
