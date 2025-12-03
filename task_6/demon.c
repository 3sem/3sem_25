#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <stdarg.h>

static int target_pid = 0;
static int period_ms = 1000;

char cwd_path[256];
char backup_dir[256] = "./backup";
char fifo_path[] = "/tmp/monfifo";

FILE *logf = NULL;

void log_error(const char *fmt, ...) {
    if (!logf) {
        mkdir("backup", 0777);
        logf = fopen("backup/error.log", "a");
        if (!logf) return;
    }

    va_list args;
    va_start(args, fmt);

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);

    fprintf(logf, "[%02d:%02d:%02d] ", tm->tm_hour, tm->tm_min, tm->tm_sec);
    vfprintf(logf, fmt, args);
    fprintf(logf, "\n");
    fflush(logf);

    va_end(args);
}

void demonize() {
    pid_t pid = fork();
    if (pid > 0) exit(0);
    setsid();
    fclose(stdin); fclose(stdout); fclose(stderr);
}

void read_pid_cwd() {
    snprintf(cwd_path, sizeof(cwd_path), "/proc/%d/cwd", target_pid);
    char buf[256];
    ssize_t len = readlink(cwd_path, buf, sizeof(buf)-1);

    if (len < 0) {
        log_error("FAILED to read cwd of PID %d", target_pid);
        return;
    }

    buf[len] = 0;
    strcpy(cwd_path, buf);
    log_error("Set CWD to: %s", cwd_path);
}

void ensure_dir(const char *path) {
    mkdir(path, 0777);
}

int is_text_file(const char *path) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "file --mime-type '%s' 2>>backup/error.log | grep -q text", path);
    int r = system(cmd);
    if (r < 0) log_error("file command failed for %s", path);
    return r == 0;
}

void save_full_backup(const char *file, const char *fullpath) {
    char dir[512], cp[512];
    snprintf(dir, sizeof(dir), "%s/%s", backup_dir, file);
    ensure_dir(dir);

    snprintf(cp, sizeof(cp), "cp '%s' '%s/0_full'", fullpath, dir);
    int r = system(cp);
    if (r != 0) log_error("FAILED full backup: %s", cp);
}

void save_incremental(const char *file, const char *fullpath) {
    char dir[512];
    snprintf(dir, sizeof(dir), "%s/%s", backup_dir, file);

    int last_diff = 0;
    DIR *d = opendir(dir);
    if (!d) {
        log_error("Cannot open backup dir: %s", dir);
        return;
    }
    struct dirent *ent;
    while ((ent = readdir(d))) {
        int n;
        if (sscanf(ent->d_name, "%d_diff", &n) == 1)
            if (n > last_diff) last_diff = n;
    }
    closedir(d);

    char prev_file[512];
    int new_diff_num = last_diff + 1;

    if (last_diff == 0) {
        snprintf(prev_file, sizeof(prev_file), "%s/0_full", dir);
    } else {
        snprintf(prev_file, sizeof(prev_file), "%s/%d_snapshot", dir, last_diff);
    }

    char tmp_diff[512];
    snprintf(tmp_diff, sizeof(tmp_diff), "%s/%d_diff_tmp", dir, new_diff_num);

    char diffcmd[1024];
    snprintf(diffcmd, sizeof(diffcmd),
             "diff -u '%s' '%s' > '%s'",
             prev_file, fullpath, tmp_diff);

    int r = system(diffcmd);
    if (r > 1) log_error("diff failed: %s", diffcmd);

    struct stat st;
    if (stat(tmp_diff, &st) == 0 && st.st_size > 0) {
        char diff_out[512];
        snprintf(diff_out, sizeof(diff_out), "%s/%d_diff", dir, new_diff_num);
        rename(tmp_diff, diff_out);

        char snapshot[512];
        snprintf(snapshot, sizeof(snapshot), "%s/%d_snapshot", dir, new_diff_num);
        char cp[512];
        snprintf(cp, sizeof(cp), "cp '%s' '%s'", fullpath, snapshot);
        int r2 = system(cp);
        if (r2 != 0) log_error("FAILED snapshot copy: %s", cp);
    } else {
        remove(tmp_diff);
    }
}


void sample() {
    DIR *d = opendir(cwd_path);
    if (!d) {
        log_error("Cannot open directory: %s", cwd_path);
        return;
    }

    struct dirent *ent;
    while ((ent = readdir(d))) {
        if (ent->d_type != DT_REG) continue;

        char full[512];
        snprintf(full, sizeof(full), "%s/%s", cwd_path, ent->d_name);

        if (!is_text_file(full)) continue;

        char dir[512];
        snprintf(dir, sizeof(dir), "%s/%s", backup_dir, ent->d_name);

        struct stat st;
        if (stat(dir, &st) != 0) {
            ensure_dir(backup_dir);
            save_full_backup(ent->d_name, full);
        } else {
            save_incremental(ent->d_name, full);
        }
    }

    closedir(d);
}


void command_loop() {
    mkfifo(fifo_path, 0666);
    int fd = open(fifo_path, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        log_error("Failed to open FIFO %s", fifo_path);
        return;
    }

    char cmd[256];
    while (1) {
        int n = read(fd, cmd, sizeof(cmd)-1);
        if (n > 0) {
            cmd[n] = 0;

            if (strncmp(cmd, "quit", 4) == 0) {
                log_error("Received quit command. Exiting.");
                unlink(fifo_path);
                exit(0);
            }
            else if (sscanf(cmd, "period %d", &period_ms) == 1) {
                log_error("Changed period to %d ms", period_ms);
            }
            else if (sscanf(cmd, "pid %d", &target_pid) == 1) {
                read_pid_cwd();
                log_error("Changed target PID to %d", target_pid);
            }
        }

        sample();
        usleep(period_ms * 1000);
    }
}


int main(int argc, char **argv) {
    int demon_flag = 0;

    int opt;
    while ((opt = getopt(argc, argv, "dp:")) != -1) {
        if (opt == 'd') demon_flag = 1;
        else if (opt == 'p') target_pid = atoi(optarg);
    }

    if (!target_pid) {
        printf("Usage: %s [-d] -p PID\n", argv[0]);
        return 1;
    }

    read_pid_cwd();

    if (demon_flag) demonize();

    ensure_dir(backup_dir);
    command_loop();

    return 0;
}
