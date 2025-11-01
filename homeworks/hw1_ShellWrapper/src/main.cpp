#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <ShellWrapper.hpp>

int main() {

	while (true) {
		system(PROMPT_STRING);

		char cmd[ARG_MAX] = {};
		read(STDIN_FILENO, cmd, sizeof(cmd));
		run_cmd(cmd);
	}

	return 0;
 }

static void run_cmd(const char *cmd) {

	if (!cmd)
		return;

	static int pipefd[2] = {};
	static int fd_in = 0;
	if (pipe(pipefd) == -1) {
        perror("pipe failed");
        return;
    }

	static int conv_cnt = -1;
	const char *new_cmd = strchr(cmd, '|');
	if (new_cmd || conv_cnt > -1)
		conv_cnt++;

	const pid_t pid = fork();
	char **args = NULL;

	Child child = {.args = args, .cmd = cmd, .conv_cnt = conv_cnt,
				   .fd_in = fd_in, .new_cmd = new_cmd, .pipefd = pipefd};

	Parent pnt = {.pid = pid, .pipefd = pipefd, .conv_cnt = &conv_cnt,
				  .fd_in = &fd_in, .args = args, .new_cmd = new_cmd};

	if (pid < 0) {
		printf("fork failed!\n");
		return;
	}

	if (pid) {
		parent_run(&pnt);
		return;
	}

	child_run(&child);
	return;
}

 char **parse_cmd(const char* cmd) {

	char **args = NULL;
	size_t words_amount = (size_t)strwords(cmd);

	args = (char**)calloc(words_amount + 1, sizeof(char*));
	if (!words_amount || !args) {
		printf("memory allocation failed!\n");
		return NULL;
	}

	size_t arg_index = 0;
	char delim[] = " |\t\n";

	char cmd_copy[ARG_MAX] = {};
	strcpy(cmd_copy, cmd);

	for (char *p = strtok(cmd_copy,delim); arg_index < words_amount; p = strtok(NULL, delim))
		args[arg_index++] = p;

	return args;
 }

 static void child_run(Child *child) {
	child->args = parse_cmd(child->cmd);

	if (child->conv_cnt > 0)
          dup2(child->fd_in, STDIN_FILENO);

	if (child->new_cmd) {
        dup2(child->pipefd[1], STDOUT_FILENO);
        close(child->pipefd[0]);
	}

	if (child->args) execvp(child->args[0], child->args);
	printf("exec* failed\n");
	return;
 }

static void parent_run(Parent *pnt) {
	int status = 0;
	waitpid(pnt->pid, &status, 0);

	close(pnt->pipefd[1]);
	if (*pnt->conv_cnt > 0)
		close(*pnt->fd_in);
	*pnt->fd_in = pnt->pipefd[0];

	if (pnt->args) {
		free(pnt->args);
		pnt->args = NULL;
	}

	int child_exit_code = WEXITSTATUS(status);
	if (child_exit_code) printf("Child process exit code: %d\n", child_exit_code);

	if (pnt->new_cmd)
		run_cmd(pnt->new_cmd + 1);

	*pnt->fd_in = 0;
	*pnt->conv_cnt = -1;
	return;
}

 int strwords(const char *string) {

	int words_count = 0;
	const char *p = string;

	while (isspace((*p))) p++;

	for (; *p != '\0'; p++) {
		if (words_count > 0 && *p == '|') {
			if (isalpha(*(p - 1))) words_count++;
			break;
		}

		if (isspace(*p)) {
			words_count++;
			while (isspace((*(p + 1)))) p++;
		}
	}

	return words_count;
 }
