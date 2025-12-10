#include "duplex_pipe.h"
#include"Rdtscp.h"

const int Tmp_buf_size = 65536;
int main() {

    Duplex_pipe* dplx_pipe = duplex_pipe_ctor(Tmp_buf_size);
    if (!dplx_pipe) 
        return -1;

    pid_t pid = fork();
    if (pid < 0) {

        DEBUG_PRINTF("fork failed!\n");
        return -1;
    }
    
    if (pid == 0) {

        dplx_pipe->methods.close_unused_pipefd(dplx_pipe, CHLD_PIPE);
        int64_t curr_size = 0;
        while ((curr_size = dplx_pipe->methods.recieve(dplx_pipe, RCV_DIR)) > 0) {

            if (dplx_pipe->methods.send(dplx_pipe, SND_REV) == Send_error_val) {

                dplx_pipe->methods.dtor(dplx_pipe);  
                exit(Send_error_val);
            }
        }
        
        if (curr_size == Recieve_error_val) {

            dplx_pipe->methods.dtor(dplx_pipe); 
            exit(Recieve_error_val);
        }

        DEBUG_PRINTF("Child->Parent transmition succeeded\n");
        dplx_pipe->methods.close_pipefd(dplx_pipe, WR_REV);
        dplx_pipe->methods.dtor(dplx_pipe);
        exit(0);
    }
    
    else {

        dplx_pipe->methods.close_unused_pipefd(dplx_pipe, PRNT_PIPE);
        int64_t curr_size_dir = 1, curr_size_rev = 1;
        uint64_t start_time = Rdtscp();
        do {

            if (curr_size_dir > 0) {  

                curr_size_dir = dplx_pipe->methods.recieve(dplx_pipe, STDIN_FILENO);
                if (curr_size_dir == Recieve_error_val) {
                
                    dplx_pipe->methods.dtor(dplx_pipe);  
                    return Recieve_error_val;
                }

                else if (curr_size_dir == 0) {

                    dplx_pipe->methods.close_pipefd(dplx_pipe, WR_DIR);
                }

                else if (dplx_pipe->methods.send(dplx_pipe, SND_DIR) == Send_error_val) {

                    dplx_pipe->methods.dtor(dplx_pipe);  
                    return Send_error_val;
                }
            }
            
            curr_size_rev = dplx_pipe->methods.recieve(dplx_pipe, RCV_REV);
            if(curr_size_rev == Recieve_error_val) {
                
                dplx_pipe->methods.dtor(dplx_pipe);  
                return Recieve_error_val;
            }

            if (curr_size_rev == 0)
                break;

            if (dplx_pipe->methods.send(dplx_pipe, STDOUT_FILENO) == Send_error_val) {

                dplx_pipe->methods.dtor(dplx_pipe);  
                return Send_error_val;
            }
        }
        while (curr_size_dir > 0 || curr_size_rev > 0); 
        
        DEBUG_PRINTF("Parent->child && Parent<-child transmition succeeded\n");
        
        uint64_t end_time = Rdtscp();
        DEBUG_PRINTF("Time consumed: %lu ms\n", end_time - start_time);
    }

    dplx_pipe->methods.dtor(dplx_pipe);

    return 0;
}
