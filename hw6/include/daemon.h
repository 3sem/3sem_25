#pragma once

#include "fifo.h"
#include "process.h"

static const char* ID_LOG = "backup_daemon";

void monitor_proc(Process* process);
void daemon_mode(Process* process);