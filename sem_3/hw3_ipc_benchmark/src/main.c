#include "common.h"
#include "fifo_transfer.h"
#include "mq_transfer.h"
#include "shm_transfer.h"

int main (int argc, char *argv[])
{
    // argv[1] - trancfer type
    // argv[2] - filename

    if (argc != 3) 
    {
        perror ("Incorrect number of arguments");
        return 1;
    }

    if (strcmp (argv[1], "shm") == 0)
    {
        shm_copy_file (argv[2]);
    }
    else if (strcmp (argv[1], "mq") == 0)
    {
        mq_copy_file (argv[2]);
    }
    else if (strcmp (argv[1], "fifo") == 0)
    {
        fifo_copy_file (argv[2]);
    }
    else
    {
        perror ("Incorrect transfer type");
        return 1;
    }

    return 0;

}