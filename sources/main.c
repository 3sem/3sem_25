#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SZ 4096

typedef struct pPipe Pipe;
typedef struct op_pipe_table Op_pipe;

typedef struct pString String;
typedef struct op_str_table Op_str;

typedef struct op_pipe_table  {
    size_t (*rcv)(Pipe *self); 
    size_t (*snd)(Pipe *self); 
} Op_pipe;

typedef struct pPipe {
        char* data;
        int fd_direct[2]; // array of r/w descriptors for "pipe()" call (for parent-->child direction)
        int fd_back[2]; // array of r/w descriptors for "pipe()" call (for child-->parent direction)
        size_t len;
        Op_pipe actions;
} Pipe;

typedef struct op_str_table  {
    int (*len)(String *self);
} Op_str;

typedef struct pString {
        char* data;
        Op_str actions;
} String;

int recieve(Pipe *self) {

}

int send(Pipe *self) {
    
}


int f_length(String *self) {
    return strlen(self->data);
}

String *String_ctor(int n) {
    String * str = malloc(sizeof(String));
    str->data = malloc(sizeof(char) * n);
    str->actions.len = f_length;

    str->data[0] = 0;

    return str;
}

int *String_dtor(String *str) {
    if (!str) return 0;

    if (str->data) free(str->data);
    free(str);

    return 0;
}

int main() {
    String *p = String_ctor(BUF_SZ);
    strcpy(p->data, "Hello");
    printf("\n%d", p->actions.len(p));
    return 0;
}