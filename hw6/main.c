#include "include/daemon.h"

int main(int argc, char** argv) 
{   
    Process process;
    
    pid_t target_pid = atoi(argv[1]);
    process.target_pid = target_pid;
    char* work_dir = get_workdir_from_procfs(target_pid);
    process.work_dir = work_dir;

    process.backup_dir = get_backupdir_from_procfs(getpid());
    
    if (argc >= 3 && !strcmp(argv[2], "-d")) 
    {
        process.output_mode = OUTPUT_SYSLOG;
        daemon_mode(&process);
    }
    else
    {
        process.output_mode = OUTPUT_TERMINAL;
        monitor_proc(&process);
    }

    free(process.work_dir);
    free(process.backup_dir);

    return 0;
}