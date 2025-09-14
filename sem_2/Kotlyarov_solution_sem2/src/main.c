#include "duplex_pipe.h"

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
        int i = 0;
        DEBUG_PRINTF("CHILD ");
        while ((curr_size = dplx_pipe->methods.recieve(dplx_pipe, RCV_DIR)) > 0) {

            DEBUG_PRINTF("CHILD ");
            if (dplx_pipe->methods.send(dplx_pipe, SND_REV) == Send_error_val) {

                dplx_pipe->methods.dtor(dplx_pipe);  
                exit(Send_error_val);
            }
           
            DEBUG_PRINTF("Child in while: %d\n", i++);
            DEBUG_PRINTF("CHILD ");
        }
        
        DEBUG_PRINTF("Child EXITED while\n");
        
        if(curr_size == Recieve_error_val) {
            dplx_pipe->methods.dtor(dplx_pipe); // Освободить до exit
            exit(Recieve_error_val);
        }

        DEBUG_PRINTF("Child->Parent transmition succeeded\n");
        dplx_pipe->methods.close_pipefd(dplx_pipe, WR_REV);
        dplx_pipe->methods.dtor(dplx_pipe);
        exit(0);
    }
    
    else {

        dplx_pipe->methods.close_unused_pipefd(dplx_pipe, PRNT_PIPE);
        
        int64_t curr_size = 0;
        int i = 0;
        DEBUG_PRINTF("PARENT ");
        while ((curr_size = dplx_pipe->methods.recieve(dplx_pipe, STDIN_FILENO)) > 0) {

            DEBUG_PRINTF("PARENT ");
            if (dplx_pipe->methods.send(dplx_pipe, SND_DIR) == Send_error_val) {

                dplx_pipe->methods.dtor(dplx_pipe);  
                return Send_error_val;
            }

            DEBUG_PRINTF("Parent in while: %d\n", i++);
            DEBUG_PRINTF("PARENT ");
        }

        DEBUG_PRINTF("Parent EXITED while\n");
        i = 0;

        if(curr_size == Recieve_error_val) {
            
            dplx_pipe->methods.dtor(dplx_pipe);  
            return Recieve_error_val;
        }

        DEBUG_PRINTF("Parent->child transmition succeeded\n");
        dplx_pipe->methods.close_pipefd(dplx_pipe, WR_DIR);
        //close(dplx_pipe->pipefd_direct[WR_PIPE_END]);
        wait(NULL);
        DEBUG_PRINTF("PARENT ");
        while ((curr_size = dplx_pipe->methods.recieve(dplx_pipe, RCV_REV)) > 0) {

            DEBUG_PRINTF("PARENT ");
            if (dplx_pipe->methods.send(dplx_pipe, STDOUT_FILENO) == Send_error_val) {

                dplx_pipe->methods.dtor(dplx_pipe);  
                return Send_error_val;
            }

            DEBUG_PRINTF("Parent in while: %d\n", i++);
            DEBUG_PRINTF("PARENT ");
        }

        DEBUG_PRINTF("Parent EXITED while\n");
        
        if(curr_size == Recieve_error_val) {
            
            dplx_pipe->methods.dtor(dplx_pipe);  
            return Recieve_error_val;
        }

        DEBUG_PRINTF("Parent<-child transmition succeeded\n");

    }

    dplx_pipe->methods.dtor(dplx_pipe);
    return 0;
}
