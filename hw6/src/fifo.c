#include "../include/fifo.h"

static void log_message(output_mode_t output_mode, char* format, ...);
static void log_command(const char* command, const char* description, output_mode_t output_mode);
static void show_current_file(Process* process);
static void show_init_file(Process* process);
static void show_prev_diff(Process* process);
static void show_tmp_diff_file(Process* process, int start_diff_sample);
static int find_first_diff_sample(Process* process, int diff_amt);
static void show_diff_for_num(Process* process, int diff_amt);
static void show_diff_for_sample(Process* process, int sample_amt);
static void change_pid(Process* process);
static void kdiff_cmd(int diff_amt, Process* process);
static void ksam_cmd(int samples_amt, Process* process);

void log_message(output_mode_t output_mode, char* format, ...) 
{
    va_list args;
    va_start(args, format);
    
    if (output_mode == OUTPUT_TERMINAL) 
    {
        vprintf(format, args);
        fflush(stdout);
    }
    else 
    {
        char buffer[MAX_LEN];
        vsnprintf(buffer, sizeof(buffer), format, args);
        syslog(LOG_INFO, "%s", buffer);
    }
    
    va_end(args);
}


void log_command(const char* command, const char* description, output_mode_t output_mode) 
{
    if (output_mode == OUTPUT_TERMINAL) 
    {
        fflush(stdout);
        
        system(command);
        
        fflush(stdout);
    }
    else 
    {
        char full_command[MAX_LEN];
        snprintf(full_command, sizeof(full_command), "%s 2>&1", command);
        
        FILE* fp = popen(full_command, "r");
        if (fp) 
        {
            char buffer[MAX_LEN];
            while (fgets(buffer, sizeof(buffer), fp) != NULL) 
            {
                buffer[strcspn(buffer, "\n")] = 0;
                syslog(LOG_INFO, "%s: %s", description, buffer);
            }
            pclose(fp);
        } 
        else 
            syslog(LOG_ERR, "Failed to execute command: %s", command);
    }
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------

int find_first_diff_sample(Process* process, int diff_amt)
{
    int sample_cnt = process->sample_cnt;
    if (diff_amt > sample_cnt)
        return 0;

    int diff_cnt = 0;
    for (int i = sample_cnt; i > 0; i--)
    {
        char diff_path[1024];
        snprintf(diff_path, sizeof(diff_path), "%s/%d.diff", process->backup_dir, i);

        if (!access(diff_path, F_OK)) 
            diff_cnt++;

        if (diff_cnt == diff_amt)
            return i;
    }

    return 0;
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------

void show_current_file(Process* process)
{
    char* file_name = process->file_name;
    char path[MAX_LEN];
    snprintf(path, sizeof(path), "%s/%s", process->work_dir, file_name);

    char command[MAX_LEN];
    snprintf(command, sizeof(command), "cat \"%s\"", path);
    char descr[MAX_LEN];
    snprintf(descr, sizeof(descr), "current file %s state", file_name);
    log_command(command, descr, process->output_mode);
}

void show_init_file(Process* process)
{
    char* file_name = process->file_name;

    char folder_name[MAX_LEN];
    strcpy(folder_name, file_name);
    
    char* dot = strrchr(folder_name, '.');
    if (dot)
        *dot = '\0'; 
    char backup_dir[MAX_LEN];
    snprintf(backup_dir, sizeof(backup_dir), "%s/%s", process->backup_dir, folder_name);

    char command[MAX_LEN];
    snprintf(command, sizeof(command), "cat \"%s/full.txt\"", backup_dir);
    char descr[MAX_LEN];
    snprintf(descr, sizeof(descr), "initial file %s state", file_name);
    log_command(command, descr, process->output_mode);
}



void show_tmp_diff_file(Process* process, int start_diff_sample)
{
    make_tmp_file_for_diff(process, start_diff_sample);

    char tmp_path[MAX_LEN];
    snprintf(tmp_path, sizeof(tmp_path), "%s/tmp.txt", process->backup_dir);

    char command[MAX_LEN];
    snprintf(command, sizeof(command), "cat \"%s\"", tmp_path);

    char descr[MAX_LEN];
    snprintf(descr, sizeof(descr), "file %s state for %d sample", process->file_name, start_diff_sample - 1);        //накручиваться диффы будут до этого диффа невключительно
    log_command(command, descr, process->output_mode);

    unlink(tmp_path);
}


void show_diff_for_num(Process* process, int diff_amt)
{
    output_mode_t output_mode = process->output_mode;
    char* file_name = process->file_name;
    char* work_dir = process->work_dir;

    char file[MAX_LEN];
    snprintf(file, sizeof(file), "%s/%s", work_dir, file_name);
    if (access(file, F_OK))
    {
        log_message(output_mode, "there are no such file %s!\n", file_name);
        return;
    } 
    
    int is_dir_exist = is_dir_exists(process->backup_dir);
    if (!is_dir_exist)
    {
        log_message(output_mode, "no changes yet! there are no diffs to show the difference\n");
        show_init_file(process);
        return;
    }

    int start_diff_sample = find_first_diff_sample(process, diff_amt);
    if (!start_diff_sample)
    {
        log_message(output_mode, "there are not enough existing diffs to show the difference\n");
        show_init_file(process);
        show_current_file(process);
        return;
    }

    show_tmp_diff_file(process, start_diff_sample);
    show_current_file(process);
}

void show_diff_for_sample(Process* process, int sample_amt)
{
    output_mode_t output_mode = process->output_mode;
    
    int is_dir_exist = is_dir_exists(process->backup_dir);
    if (!is_dir_exist)
    {
        log_message(output_mode, "no changes yet! there are no diffs to show the difference\n");
        show_init_file(process);
        return;
    }

    int sample_cnt = process->sample_cnt;
    if (sample_cnt < sample_amt)
    {
        log_message(output_mode, "there are not enough existing samples to show the difference\n");
        show_init_file(process);
        show_current_file(process);
        return;
    }

    show_tmp_diff_file(process, sample_cnt - sample_amt + 1);
    show_current_file(process);
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------

void show_prev_diff(Process* process)
{
    char* file_name = process->file_name;
    char* work_dir = process->work_dir;
    output_mode_t output_mode = process->output_mode;

    char file[MAX_LEN];
    snprintf(file, sizeof(file), "%s/%s", work_dir, file_name);
    if (access(file, F_OK))
    {
        log_message(output_mode, "there are no such file %s!\n", file_name);
        return;
    } 

    int is_dir_exist = is_dir_exists(process->backup_dir);
    if (!is_dir_exist)
    {
        log_message(output_mode, "no changes yet!\n");
        show_current_file(process);
        return;
    }

    int start_sample = find_first_diff_sample(process, 1);
    show_tmp_diff_file(process, start_sample);
    show_current_file(process);
}

void change_pid(Process* process)
{
    char command[MAX_LEN];
    snprintf(command, sizeof(command), "rm -rf '%s'", process->backup_dir); 
    system(command);

    free(process->work_dir);
    process->work_dir = get_workdir_from_procfs(process->target_pid);

    inotify_rm_watch(process->inotify_fd, 1); 
    inotify_add_watch(process->inotify_fd, process->work_dir, IN_CREATE | IN_DELETE | IN_MODIFY);
    create_directory(process->backup_dir);                                                              //ее не меняем, просто удалили, чтобы не засорять. местоположение рабочей папки (не для процесса) такое же, поэтому директория просто пересоздастся
    log_message(process->output_mode, "target pid is changed");
    process->sample_cnt = 0;
}

void kdiff_cmd(int diff_amt, Process* process)
{
    output_mode_t output_mode = process->output_mode;
    if (!diff_amt)
    {
        log_message(output_mode, "0 diffs is current file\n");
        show_current_file(process);
    }
    else
        show_diff_for_num(process, diff_amt);
}

void ksam_cmd(int samples_amt, Process* process)
{
    output_mode_t output_mode = process->output_mode;
    if (!samples_amt)
    {
        log_message(output_mode, "0 samples is current file\n");
        show_current_file(process);
    }
    else
        show_diff_for_sample(process, samples_amt);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

void process_command(Process* process, const char* command)
{
    char cmd[MAX_LEN];
    strncpy(cmd, command, sizeof(cmd) - 1);
    cmd[sizeof(cmd) - 1] = '\0';

    for (char* p = cmd; *p; p++) 
    {
        if (*p == '\n' || *p == '\r') 
        {
            *p = '\0';
            break;
        }
    }

    output_mode_t output_mode = process->output_mode;
    char* work_dir = process->work_dir;

    log_message(output_mode, "entered command: %s\n", cmd);

    int token_cnt = 0;
    char* tokens[10];
    char* token = strtok(cmd, " ");
    while (token != NULL && token_cnt < 3) 
    {
        tokens[token_cnt++] = token;
        token = strtok(NULL, " ");
    }

    char* the_cmd = tokens[0];
    char* init_backup_dir = process->backup_dir;

    if (!strcmp(the_cmd, KDIFF_CMD)) 
    {
        process->file_name = tokens[2];
        specialize_backup_dir_for_file(process);
        char* backup_dir = process->backup_dir;
        int diff_amt = atoi(tokens[1]);
        kdiff_cmd(diff_amt, process);
    }
    else if (!strcmp(the_cmd, KSAM_CMD))
    {
        process->file_name = tokens[2];
        specialize_backup_dir_for_file(process);
        char* backup_dir = process->backup_dir;
        int samples_amt = atoi(tokens[1]);
        ksam_cmd(samples_amt, process);
    }
    else if (!strcmp(cmd, PID_CMD))
    {
        pid_t new_pid = atoi(tokens[1]);
        process->target_pid = new_pid;
        change_pid(process);
    }
    else if (!strcmp(the_cmd, PREVDIFF_CMD)) 
    {
        process->file_name = tokens[1];
        specialize_backup_dir_for_file(process);
        char* backup_dir = process->backup_dir;
        show_prev_diff(process);
    }   
    else if (!strcmp(the_cmd, SAMPLE_CMD)) 
        log_message(output_mode, "current sample is %d\n", process->sample_cnt);
    else if (!strcmp(cmd, EXIT_CMD)) 
    {
        log_message(output_mode, "goodbye, friend\n");
        exit(0);
    }
    else 
        log_message(output_mode, "you are wrong. you entered wrong command\n");

    return_init_backup_dir(init_backup_dir, process);
}