#ifndef CLIENT_FUNCS
#define CLIENT_FUNCS

typedef struct {
    int tx_fd; 
    int rx_fd;
    char tx_path[128];
    char rx_path[128];
    int client_number;
    int registered;
} client_t;

#endif
