#ifndef TRANS_FUNCS_TEST
#define TRANS_FUNCS_TEST

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <time.h>
#include <stdbool.h>
#include "Debug_printf.h"

#define BUF_SIZE (8192)

struct shared_data {
    char buffer[BUF_SIZE];
    size_t buf_size;
    char eof_flag;  // if 1 => eof
};

struct mqbuf {

    long mtype;
    char mtext[BUF_SIZE];
};

#define FIFO_NAME "FIFO_transmission_pipe"
#define EOF_MSG "EOF"
#define GNUPLOT_HEADER \
    "set title \"Сравнение методов IPC: %s\"\n" \
    "set xlabel \"Метод передачи\"\n" \
    "set ylabel \"Время (секунды)\"\n" \
    "set style data histograms\n" \
    "set style histogram cluster gap 1\n" \
    "set style fill solid border -1\n" \
    "set boxwidth 0.9\n" \
    "set xtic rotate by -45 scale 0\n" \
    "set grid y\n" \
    "set key autotitle columnheader\n" \
    "set yrange [0:*]\n" \
    "set terminal png size 800,600\n" \
    "set output 'plot_%s.png'\n" \
    "plot '%s' using 2:xticlabels(1) with histograms fill solid lc rgb \"blue\"\n" \
    "set output\n"

int Fifo_transmission(char* input_filename, char* output_filename, double* time_consumed);
int SYS_V_mq_transmissio(char* input_filename, char* output_filename, double* time_consumed);
bool sem_w8_(int semid, int sem_num);
bool sem_post_(int semid, int sem_num);
int SYS_V_shm_transmission(char* input_filename, char* output_filename, double* time_consumed);
void generate_gnuplot_script(double fifo_times[3], double mq_times[3], double shm_times[3]);
#endif