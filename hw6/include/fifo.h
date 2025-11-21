#pragma once

#include "process.h"
#include <stdarg.h>
#include <syslog.h>

#define FIFO_PATH "/tmp/backup_daemon_fifo"

static const char* KDIFF_CMD = "diffback";
static const char* PREVDIFF_CMD = "prevdiff";
static const char* PID_CMD = "newpid";
static const char* EXIT_CMD = "exit";
static const char* KSAM_CMD = "sampleback";
static const char* SAMPLE_CMD = "sample";

void process_command(Process* process, const char* command);