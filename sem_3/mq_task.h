#ifndef MQ_TASK_H
#define MQ_TASK_H

#include <mqueue.h>

#define SERVER_QUEUE_NAME   "/file-transfer-server"
#define QUEUE_PERMISSIONS   0660
#define MAX_MESSAGES        10
#define MAX_MSG_SIZE        1048566
#define MSG_BUFFER_SIZE     (MAX_MSG_SIZE + 10)

#endif

