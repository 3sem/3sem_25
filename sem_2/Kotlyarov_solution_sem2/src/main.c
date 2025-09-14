
#include "duplex_pipe.h"

const int Tmp_buf_size = 4096;
int main() {

    Duplex_pipe* dplx_pipe = duplex_pipe_ctor(Tmp_buf_size);
    pid_t pid = fork();
    if (pid < 0) {

        DEBUG_PRINTF("fork failed!\n");
        return -1;
    }
    
    if (pid == 0) {

        dplx_pipe->methods.close_unused_pipefd(dplx_pipe, CHLD_PIPE);
        int64_t total_size = 0;
        for(
            int64_t curr_size = 0; 
            (curr_size = dplx_pipe->methods.recieve(dplx_pipe, RCV_DIR));
            total_size += curr_size
        ) 
        {

            if(curr_size == Recieve_error_val) 
                exit(Recieve_error_val);

            if (dplx_pipe->methods.send(dplx_pipe, SND_REV) == Send_error_val)
                exit(Send_error_val);
        }
        
        DEBUG_PRINTF("Child->Parent transmition succeeded\n");
        exit(0);
    }
    
    else {

        dplx_pipe->methods.close_unused_pipefd(dplx_pipe, PRNT_PIPE);
        
        int64_t total_size = 0;
        for(
            int64_t curr_size = 0; 
            (curr_size = dplx_pipe->methods.recieve(dplx_pipe, STDIN_FILENO));
            total_size += curr_size
        ) 
        {

            if(curr_size == Recieve_error_val) 
                return Recieve_error_val;

            if (dplx_pipe->methods.send(dplx_pipe, SND_DIR) == Send_error_val)
                return Send_error_val;
        }
        
        DEBUG_PRINTF("Parent->child transmition succeeded\n");
        dplx_pipe->methods.close_pipefd(dplx_pipe, WR_DIR);

        total_size = 0;
        for(
            int64_t curr_size = 0; 
            (curr_size = dplx_pipe->methods.recieve(dplx_pipe, RCV_REV));
            total_size += curr_size
        ) 
        {

            if (curr_size == Recieve_error_val) 
                return Recieve_error_val;

            if (dplx_pipe->methods.send(dplx_pipe, STDOUT_FILENO) == Send_error_val)
                return Send_error_val;
        }

        DEBUG_PRINTF("Parent<-child transmition succeeded\n");

    }

    dplx_pipe->methods.dtor(dplx_pipe);
    return 0;
}
