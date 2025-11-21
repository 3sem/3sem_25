#include "../include/daemon.h"

void daemon_mode(Process* process)
{
    pid_t pid = fork();
    
    if (pid > 0)
        exit(0);

    setsid();
    umask(0);
    chdir("/");
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    int null_fd = open("/dev/null", O_RDWR);
    dup2(null_fd, STDIN_FILENO);
    dup2(null_fd, STDOUT_FILENO);
    dup2(null_fd, STDERR_FILENO);
    close(null_fd);
    
    monitor_proc(process);
    closelog();
}

void monitor_proc(Process* process)
{
    int inotify_fd = inotify_init();
    process->inotify_fd = inotify_fd;
    fcntl(inotify_fd, F_SETFL, O_NONBLOCK);

    create_directory(process->backup_dir);
    mkfifo(FIFO_PATH, 0666);

    inotify_add_watch(inotify_fd, process->work_dir, IN_CREATE | IN_MODIFY);
    
    char buffer[BUF_SIZE];
    int fifo_fd = open(FIFO_PATH, O_RDONLY | O_NONBLOCK);

    process->sample_cnt = 0;

    while (1) 
    {
        sleep(SAMPLE_TIME);
        int num_bytes = read(inotify_fd, buffer, BUF_SIZE);
    
        if (num_bytes > 0)
            process_events(process, buffer, num_bytes);

        char command[MAX_LEN];
        int bytes_read = 0;
        while ((bytes_read = read(fifo_fd, command, sizeof(command) - 1)) > 0) 
        {
            command[bytes_read] = '\0';
            process_command(process, command);
        }

        process->sample_cnt++;
    }

    close(fifo_fd);
    close(inotify_fd);
    unlink(FIFO_PATH);
}