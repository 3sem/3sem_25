#include <duplex_pipe.h>

Duplex_pipe* duplex_pipe_ctor(size_t tmp_buf_capacity) { 

    Duplex_pipe* dplx_pipe = (Duplex_pipe*) calloc(1, sizeof(Duplex_pipe));
    if (!dplx_pipe) {

        DEBUG_PRINTF("ERROR: memory was not allocated\n");
        return NULL;
    }

    if (tmp_buf_capacity <= 0)
        dplx_pipe->tmp_buf_capacity = Tmp_buf_def_size;

    else
        dplx_pipe->tmp_buf_capacity = tmp_buf_capacity;

    dplx_pipe->tmp_buf = (char*) calloc(tmp_buf_capacity, sizeof(char));
    if (!dplx_pipe->tmp_buf) {

        DEBUG_PRINTF("ERROR: memory was not allocated\n");
        free(dplx_pipe);
        return NULL;
    }

    if (pipe(dplx_pipe->pipefd_direct) == -1) {
        
        perror("ERROR: pipe failed\n");
        free(dplx_pipe->tmp_buf);
        free(dplx_pipe);
        return NULL;
    }

    if (pipe(dplx_pipe->pipefd_reverse) == -1) {

        perror("ERROR: pipe failed\n");
        free(dplx_pipe->tmp_buf);
        free(dplx_pipe);
        return NULL;
    }
    
    dplx_pipe->methods.recieve = duplex_pipe_recieve;
    dplx_pipe->methods.send = duplex_pipe_send;
    dplx_pipe->methods.close_pipefd = duplex_pipe_close_pipefd;
    dplx_pipe->methods.close_unused_pipefd = duplex_pipe_close_unused_pipefd;
    dplx_pipe->methods.dtor = duplex_pipe_dtor;
    return dplx_pipe;
}

void duplex_pipe_close_unused_pipefd(Duplex_pipe* dplx_pipe, int pipefd_opt) { 

    if (pipefd_opt == PRNT_PIPE) {

        dplx_pipe->methods.close_pipefd(dplx_pipe, RD_DIR);
        dplx_pipe->methods.close_pipefd(dplx_pipe, WR_REV);
    }

    else if (pipefd_opt == CHLD_PIPE){

        dplx_pipe->methods.close_pipefd(dplx_pipe, WR_DIR);
        dplx_pipe->methods.close_pipefd(dplx_pipe, RD_REV);
    }

    else {

        DEBUG_PRINTF("WARNING: wrong close_unused_pipefd option\n");
    }
}

bool duplex_pipe_close_pipefd(Duplex_pipe* dplx_pipe, int pipefd_opt) { 

    if (!dplx_pipe) {

        DEBUG_PRINTF("ERROR: null ptr\n");
        return false;
    }

    if (pipefd_opt < RD_DIR || pipefd_opt > WR_REV) {
        
        DEBUG_PRINTF("ERROR: wrong pipefd option\n");
        return false;
    }
        
    int close_status = 0;
    if (pipefd_opt < RD_REV)    
        close_status = close(dplx_pipe->pipefd_direct [pipefd_opt % 2]);
        
    else 
        close_status = close(dplx_pipe->pipefd_reverse[pipefd_opt % 2]);
   
    if (close_status == -1) {
        
        DEBUG_PRINTF("ERROR: close failed\n");
        return false;
    }
        
    return true;
}

int64_t duplex_pipe_recieve(Duplex_pipe* dplx_pipe, int recieve_opt) { 

    if (!dplx_pipe) {

        DEBUG_PRINTF("ERROR: null ptr\n");
        return Recieve_error_val;
    }

    int recieve_pipefd = 0; 
    if (recieve_opt == RCV_DIR) 
        recieve_pipefd = dplx_pipe->pipefd_direct[RD_PIPE_END];

    else if (recieve_opt == RCV_REV) 
        recieve_pipefd = dplx_pipe->pipefd_reverse[RD_PIPE_END];

    else
        recieve_pipefd = recieve_opt;

    ssize_t curr_size = read(recieve_pipefd, dplx_pipe->tmp_buf, dplx_pipe->tmp_buf_capacity - 1);
    
    if(curr_size == -1) {
        
        DEBUG_PRINTF("ERROR: recieve failed\n");
        return Recieve_error_val;
    }

    if (curr_size > 0) {
        dplx_pipe->tmp_buf[curr_size] = 0;
        dplx_pipe->tmp_buf_size = curr_size;
        //DEBUG_PRINTF("Recieve: \n%sEND\n", dplx_pipe->tmp_buf);
    }

    DEBUG_PRINTF("Recieve: total_size = %ld\n", curr_size);
    return curr_size;
}

int64_t duplex_pipe_send(Duplex_pipe* dplx_pipe, int send_opt) { 

    if (!dplx_pipe) {

        DEBUG_PRINTF("ERROR: null ptr\n");
        return Recieve_error_val;
    }

    int send_pipefd = 1; 
    if (send_opt == SND_DIR) 
        send_pipefd = dplx_pipe->pipefd_direct[WR_PIPE_END];

    else if (send_opt == SND_REV) 
        send_pipefd = dplx_pipe->pipefd_reverse[WR_PIPE_END];
        
    else
        send_pipefd = send_opt;

    ssize_t curr_size = write(send_pipefd, dplx_pipe->tmp_buf, dplx_pipe->tmp_buf_size);
    
    if(curr_size == -1) {
        
        DEBUG_PRINTF("ERROR: send failed\n");
        return Send_error_val;
    }

    //DEBUG_PRINTF("Send: \n%.*sEND\n", (int)curr_size, dplx_pipe->tmp_buf);
    DEBUG_PRINTF("Send: total_size = %ld\n", curr_size);
    return curr_size;
}

bool duplex_pipe_dtor(Duplex_pipe* dplx_pipe) { 

    if (!dplx_pipe) {

        DEBUG_PRINTF("ERROR: null ptr\n");
        return false;
    }

    free(dplx_pipe->tmp_buf);
    free(dplx_pipe);
    return true;
}
