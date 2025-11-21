#include "common.h"

int main() 
{
    struct message msg;
    int msgid = msgget(KEY, 0666| IPC_CREAT);

    int fd = open(OUT_FILE_NAME, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    int complete = 0;

    while (!complete) 
    {
        memset(&msg, 0, sizeof(msg));
        msgrcv(msgid, &msg, sizeof(msg) - sizeof(long), 0, 0);

        if (msg.mtype == MSG_DATA) 
            write(fd, msg.data, msg.data_size);
        else if (msg.mtype == MSG_END) 
            complete = 1;
            
        struct message ack_msg;
        ack_msg.mtype = MSG_ACK;
        ack_msg.data_size = 0;
        msgsnd(msgid, &ack_msg, sizeof(ack_msg) - sizeof(long), 0);
    }
    close(fd);

    msgctl(msgid, IPC_RMID, NULL);
    
    return 0;
}