#pragma once

#ifdef __APPLE__
	#include <_ctype.h>
#else
	#include <ctype.h>
#endif

#define ARG_MAX 2097152

#define ESC_BOLD 			"\\033[1m"
#define ESC_BRIGHT_RED 		"\\033[91m"
#define ESC_BRIGHT_YELLOW 	"\\033[93m"
#define ESC_RESET 			"\\033[0m"

#define PROMPT_STRING		"echo \"" ESC_BOLD ESC_BRIGHT_RED "$USER@$HOSTNAME" ESC_RESET ":" ESC_BOLD ESC_BRIGHT_YELLOW "$PWD" ESC_RESET "$ \\c\""

struct Child {
	char **args;
	const char *cmd;
	int conv_cnt;
	int fd_in;
	const char *new_cmd;
	int *pipefd;
};

struct Parent {
	pid_t pid;
	int *pipefd;
	int *conv_cnt;
	int *fd_in;
	char **args;
	const char *new_cmd;
};

int strwords(const char *string);
char **parse_cmd(const char *cmd);
static void run_cmd(const char *cmd);
static void child_run(Child *child);
static void parent_run(Parent *pnt);
