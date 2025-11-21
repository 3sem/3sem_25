#include "../include/process.h"


static int is_text_file(const char* filename);
static void handle_create(Process* process);
static void handle_modify(Process* process);



void create_directory(const char* path) 
{
    mkdir(path, 0755);
}

char* get_workdir_from_procfs(pid_t pid) 
{
    char* work_dir = (char*)calloc(1024, sizeof(char));
    char proc_path[64];
    
    snprintf(proc_path, sizeof(proc_path), "/proc/%d/cwd", pid);
    
    ssize_t len = readlink(proc_path, work_dir, 1024 - 1);

    if (len > 0)
        work_dir[len] = '\0';
    else
        strcpy(work_dir, ".");
    
    return work_dir;
}

char* get_backupdir_from_procfs(pid_t pid) 
{
    char tmp[MAX_LEN];
    char* backup_dir = (char*)calloc(MAX_LEN, sizeof(char));
    char proc_path[64];
    
    snprintf(proc_path, sizeof(proc_path), "/proc/%d/cwd", pid);
    
    ssize_t len = readlink(proc_path, tmp, MAX_LEN - 1);
    if (len > 0) {
        tmp[len] = '\0';
    } else {
        free(backup_dir);
        return strdup(".");
    }
    
    snprintf(backup_dir, MAX_LEN, "%s/%s", tmp, BACKUP_DIR);
    
    return backup_dir;
}


int is_text_file(const char* filename) 
{
    const char* ext = strrchr(filename, '.');
    if (ext) 
    {
        const char* text_extensions[] = {".txt", ".c", ".h", ".cpp", ".java", 
                                       ".py", ".js", ".html", ".css", ".xml", 
                                       ".json", ".md", NULL};
        for (int i = 0; text_extensions[i]; i++) 
        {
            if (strcmp(ext, text_extensions[i]) == 0)
                return 1;
        }
    }
    
    char command[512];
    snprintf(command, sizeof(command), 
             "file -b \"%s\" | grep -q -E 'text|ASCII|UTF'", filename);
    return system(command) == 0;
}

int is_dir_exists(const char* path) 
{
    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

void specialize_backup_dir_for_file(Process* process)
{
    char folder_name[MAX_LEN];
    strcpy(folder_name, process->file_name);
    char* dot = strrchr(folder_name, '.');
    if (dot)
        *dot = '\0'; 
    
    char* backup_dir = (char*)calloc(MAX_LEN, sizeof(char));
    snprintf(backup_dir, MAX_LEN, "%s/%s", process->backup_dir, folder_name);

    process->backup_dir = backup_dir;
}

void return_init_backup_dir(char* init_backup_dir, Process* process)
{
    if (process->backup_dir == init_backup_dir)
        return;

    free(process->backup_dir);
    process->backup_dir = init_backup_dir;
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------

void make_tmp_file_for_diff(Process* process, int sample_cnt)
{
    char* backup_dir = process->backup_dir;
    char command[MAX_LEN];
    snprintf(command, sizeof(command), "touch \"%s/tmp.txt\"", backup_dir);
    system(command);
    
    snprintf(command, sizeof(command), "cp \"%s/full.txt\" \"%s/tmp.txt\"", backup_dir, backup_dir);
    system(command);

    for (int i = 1; i < sample_cnt; i++)
    {
        char diff_path[MAX_LEN];
        snprintf(diff_path, sizeof(diff_path), "%s/%d.diff", backup_dir, i);

        if (access(diff_path, F_OK) == 0) 
        {   
            char command[MAX_LEN];
            snprintf(command, sizeof(command), "patch --quiet -i \"%s\" \"%s/tmp.txt\"", diff_path, backup_dir);
            system(command);
        }
        else
            continue;
    }
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------

void handle_create(Process* process)
{
    char* init_backup_dir = process->backup_dir;
    specialize_backup_dir_for_file(process);

    create_directory(process->backup_dir);

    char command[MAX_LEN];
    snprintf(command, sizeof(command), "cp \"%s/%s\" \"%s/full.txt\"", process->work_dir, process->file_name, process->backup_dir);
    system(command);

    return_init_backup_dir(init_backup_dir, process);
}

void handle_modify(Process* process)
{
    char* file_name = process->file_name;
    int sample_cnt = process->sample_cnt;
    char* work_dir = process->work_dir;

    char* init_backup_dir = process->backup_dir;
    specialize_backup_dir_for_file(process);
    char* backup_dir = process->backup_dir;

    int is_dir_exist = is_dir_exists(backup_dir);

    if (!is_dir_exist)
    {
        create_directory(backup_dir);

        char command[MAX_LEN];
        snprintf(command, sizeof(command), "cp \"%s/%s\" \"%s/full.txt\"", work_dir, file_name, backup_dir);
        system(command);
    }
    else
    {
        make_tmp_file_for_diff(process, sample_cnt);

        char tmp_path[MAX_LEN];
        snprintf(tmp_path, sizeof(tmp_path), "%s/tmp.txt", backup_dir);

        char command[MAX_LEN];
        snprintf(command, sizeof(command), "diff -u \"%s\" \"%s/%s\" > \"%s/%d.diff\"", tmp_path, work_dir, file_name, backup_dir, sample_cnt);
        system(command);

        unlink(tmp_path);
    }

    return_init_backup_dir(init_backup_dir, process);
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------

void process_events(Process* process, char* buffer, int length)
{
    int i = 0;
    
    while (i < length) 
    {
        struct inotify_event* event = (struct inotify_event*)&buffer[i];

        if (event->name[0] == '.') 
            return;

        int is_text = is_text_file(event->name);
        if (!is_text)
            return;

        process->file_name = event->name;
        char* init_backup_dir = process->backup_dir;
        
        if (event->mask & IN_CREATE) 
            handle_create(process);
        else if (event->mask & IN_MODIFY) 
            handle_modify(process);
        
        i += sizeof(struct inotify_event) + event->len;
    }
}