#include "include/fifo.h"

void send_command(const char* command) 
{
    int fifo_fd = open(FIFO_PATH, O_WRONLY);
    
    char full_command[256];
    full_command[0] = '\0';
    snprintf(full_command, sizeof(full_command), "%s\n", command);
    strcat(full_command, "\n");
    
    size_t bytes_written = write(fifo_fd, full_command, strlen(full_command));
    
    close(fifo_fd);
}

int main() 
{   
    char* line = NULL;
    size_t len = 0;
    ssize_t read;
    
    while (1) 
    {
        printf("> ");
        read = getline(&line, &len, stdin);
        
        if (read == -1)
            break;
        
        if (read > 0 && line[read - 1] == '\n')
            line[read - 1] = '\0';
        
        if (strcmp(line, "-q") == 0)
            break;
        else
            send_command(line);
    }
    
    if (line)
        free(line);
    return 0;
}