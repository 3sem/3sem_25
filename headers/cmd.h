#ifndef CMD_CLASS
#define CMD_CLASS

#define CMD_BUFF_SIZE 512
#define SPLIT_CMD_CHAR '|'
#define SPLIT_ARG_CHAR ' '

typedef struct {
    char** argv;
    char * start;
} cmd_t;

typedef struct {
    cmd_t* cmd;
    char*  str;
    int cmd_num;
} cmd_big_t;

int run_cmd_big(cmd_big_t * list);

cmd_big_t * str_to_cmd_big(const char * cmd_str);

int dtor_cmd_big(cmd_big_t * list);

#endif
