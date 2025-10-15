#include "Transmission_funcs_test.h"

int Fifo_transmission(char* input_filename, char* output_filename, double* time_consumed) {

    int fd_input_file = open(input_filename, O_RDONLY);
    if (fd_input_file == -1) {

        perror("open");
        return 1;
    }

    int fd_output_file = 1;
    if (output_filename) {

        fd_output_file = open(output_filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd_output_file == -1) {

            perror("open");
            //fd_output_file = 1;
            return 1;
        }
    }

    char* buf = (char*) calloc(BUF_SIZE, sizeof(char));
    if (!buf) {

        perror("calloc");
        close(fd_input_file);
        if (fd_output_file != 1)
            close(fd_output_file);
        return 1;
    }

    unlink(FIFO_NAME);

    if(mknod(FIFO_NAME, S_IFIFO | 0666, 0) == -1) {
        perror("mknod");
        close(fd_input_file);
        if (fd_output_file != 1)
            close(fd_output_file);
        free(buf);
        return 1;
    }

    struct timespec start, end;
    pid_t pid = fork();
    if (pid < 0) {

        DEBUG_PRINTF("fork failed!\n");
        close(fd_input_file);
        if (fd_output_file != 1)
            close(fd_output_file);
        return -1;
    }

    if (pid == 0) {

        int ret_val = 0;
        int fd_fifo = open(FIFO_NAME, O_RDONLY);
        if (fd_fifo == -1) {

            perror("open");
            return -1;
        }

        ssize_t curr_size_read = 0;
        while ((curr_size_read = read(fd_fifo, buf, BUF_SIZE)) > 0) {

            ssize_t bytes_written_total = 0;
            while (bytes_written_total < curr_size_read) {

                ssize_t curr_size_write = write(fd_output_file, buf + bytes_written_total, curr_size_read - bytes_written_total);
                if (curr_size_write == -1) {

                    perror("write");
                    ret_val = 1;
                    break;
                }

                bytes_written_total += curr_size_write;
            }
        }

        close(fd_fifo);
        free(buf);
        unlink(FIFO_NAME);
        exit(ret_val);
    }

    else {

        int ret_val = 0;
        int fd_fifo = open(FIFO_NAME, O_WRONLY);
        if (fd_fifo == -1) {

            perror("open");
            close(fd_input_file);
            if (fd_output_file != 1)
                close(fd_output_file);
            return -1;
        }
        
        ssize_t curr_size_read = 0;
        clock_gettime(CLOCK_MONOTONIC, &start);
        while ((curr_size_read = read(fd_input_file, buf, BUF_SIZE)) > 0) {

            ssize_t bytes_written_total = 0;
            while (bytes_written_total < curr_size_read) {

                ssize_t curr_size_write = write(fd_fifo, buf + bytes_written_total, curr_size_read - bytes_written_total);
                if (curr_size_write == -1) {

                    perror("write");
                    ret_val = 1;
                    break;
                }

                bytes_written_total += curr_size_write;
            }
        }

        close(fd_input_file);
        if (fd_output_file != 1)
            close(fd_output_file);

        close(fd_fifo);
        wait(NULL);
        clock_gettime(CLOCK_MONOTONIC, &end);
        double time_taken = (end.tv_sec - start.tv_sec) + 
                            (end.tv_nsec - start.tv_nsec) / 1e9;
        fprintf(stderr, "%s: Time consumed = %.6f seconds\n", input_filename, time_taken);
        *time_consumed = time_taken;
        if (curr_size_read == -1) {

            perror("read");
            return 1;
        }

        free(buf);
        unlink(FIFO_NAME);
        return ret_val;
    }

    return 0;
}

int SYS_V_mq_transmissio(char* input_filename, char* output_filename, double* time_consumed) {

    int fd_input_file = open(input_filename, O_RDONLY);
    if (fd_input_file == -1) {

        perror("open");
        return 1;
    }

    int fd_output_file = 1;
    if (output_filename) {

        fd_output_file = open(output_filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd_output_file == -1) {

            perror("open");
            //fd_output_file = 1;
            return 1;
        }
    }

    key_t eof_flag_key = ftok(".", 'F');
    if (eof_flag_key == -1) {

        perror("ftok (shm)");
        close(fd_input_file);
        if (fd_output_file != 1)
            close(fd_output_file);
        return 1;
    }

    int shmid = shmget(eof_flag_key, sizeof(int), IPC_CREAT | 0666);
    if (shmid == -1) {

        perror("shmget");
        close(fd_input_file);
        if (fd_output_file != 1)
            close(fd_output_file);
        return 1;
    }

    int* eof_flag_ptr = (int*) shmat(shmid, NULL, 0);
    if (eof_flag_ptr == (void*) -1) {

        perror("shmat");
        close(fd_input_file);
        if (fd_output_file != 1)
            close(fd_output_file);
        return 1;
    }

    *eof_flag_ptr = 0;

    key_t mq_key = ftok(".", 'Q');
    if (mq_key == -1) {

        perror("ftok (shm)");
        close(fd_input_file);
        if (fd_output_file != 1)
            close(fd_output_file);
        shmdt(eof_flag_ptr);
        shmctl(shmid, IPC_RMID, NULL);
        return 1;
    }

    int mqid = msgget(mq_key, IPC_CREAT | 0660);
    if (mqid == -1) {

        perror("shmget");
        close(fd_input_file);
        if (fd_output_file != 1)
            close(fd_output_file);
        shmdt(eof_flag_ptr);
        shmctl(shmid, IPC_RMID, NULL);
        return 1;
    }

    struct mqbuf msg = { .mtype = 1};
    struct timespec start, end;
    
    pid_t pid = fork();
    if (pid < 0) {

        DEBUG_PRINTF("fork failed!\n");
        perror("shmget");
        close(fd_input_file);
        if (fd_output_file != 1)
            close(fd_output_file);
        shmdt(eof_flag_ptr);
        shmctl(shmid, IPC_RMID, NULL);
        msgctl(mqid, IPC_RMID, NULL);
        return -1;
    }

    if (pid == 0) {

        while (1) {

            ssize_t curr_size = msgrcv(mqid, &msg, BUF_SIZE, 0, 0);

            if(curr_size < 0) {

                perror("msgrcv");
                shmdt(eof_flag_ptr);
                exit(1);
            }

            else if(*eof_flag_ptr && !memcmp(msg.mtext, EOF_MSG, sizeof(EOF_MSG))) {

                break;
            }
                
            else {

                if(write(fd_output_file, msg.mtext, curr_size) < 0) {

                    perror("write");
                    shmdt(eof_flag_ptr);
                    exit(1);
                }
            }
        }

        shmdt(eof_flag_ptr);
        exit(0);
    }

    else {

        int64_t curr_size = 0;
        clock_gettime(CLOCK_MONOTONIC, &start);
        while ((curr_size = read(fd_input_file, msg.mtext, BUF_SIZE)) > 0) {

            if(msgsnd(mqid, &msg, curr_size, 0)) {

                perror("msgsnd");
                break;
            }
        }

        *eof_flag_ptr = 1;
        memcpy(msg.mtext, EOF_MSG, sizeof(EOF_MSG));
        msgsnd(mqid, &msg, sizeof(EOF_MSG), 0);

        close(fd_input_file);
        if (fd_output_file != 1)
            close(fd_output_file);
        wait(NULL);
        clock_gettime(CLOCK_MONOTONIC, &end);
        double time_taken = (end.tv_sec - start.tv_sec) + 
                            (end.tv_nsec - start.tv_nsec) / 1e9;
        fprintf(stderr, "%s: Time consumed = %.6f seconds\n", input_filename, time_taken);
        *time_consumed = time_taken;
        shmdt(eof_flag_ptr);
        shmctl(shmid, IPC_RMID, NULL);
        msgctl(mqid, IPC_RMID, NULL);
        if (curr_size == -1) {

            perror("read");
            return 1;
        }
    }

    return 0;
}

bool sem_w8_(int semid, int sem_num) { // sem_wAIT (wEIGHT)))), just to rename
    struct sembuf sb;
    sb.sem_num = sem_num;
    sb.sem_op = -1;  // wait untill > 0
    sb.sem_flg = 0;
    if (semop(semid, &sb, 1) == -1) {

        perror("semop sem_w8_");
        return false;
    }

    return true;
}

bool sem_post_(int semid, int sem_num) {
    struct sembuf sb;
    sb.sem_num = sem_num;
    sb.sem_op = 1;  // incr sem
    sb.sem_flg = 0;
    if (semop(semid, &sb, 1) == -1) {

        perror("semop sem_post_");
        return false;
    }

    return true;
}

int SYS_V_shm_transmission(char* input_filename, char* output_filename, double* time_consumed) {

    int fd_input_file = open(input_filename, O_RDONLY);
    if (fd_input_file == -1) {

        perror("open");
        return 1;
    }

    int fd_output_file = 1;
    if (output_filename) {

        fd_output_file = open(output_filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd_output_file == -1) {

            perror("open");
            //fd_output_file = 1;
            return 1;
        }
    }

    key_t shm_key = ftok(".", 'M');
    if (shm_key == -1) {
        perror("ftok (shm)");
        close(fd_input_file);
        if (fd_output_file != 1)
            close(fd_output_file);
        return 1;
    }

    int shmid = shmget(shm_key, sizeof(struct shared_data), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        close(fd_input_file);
        if (fd_output_file != 1)
            close(fd_output_file);
        return 1;
    }

    struct shared_data* shm_ptr = (struct shared_data*) shmat(shmid, NULL, 0);
    if (shm_ptr == (void*) -1) {
        perror("shmat");
        close(fd_input_file);
        if (fd_output_file != 1)
            close(fd_output_file);
        return 1;
    }

    key_t sem_key = ftok(".", 'S'); 
    if (sem_key == -1) {
        perror("ftok (sem)");
        shmdt(shm_ptr);
        close(fd_input_file);
        if (fd_output_file != 1)
            close(fd_output_file);
        return 1;
    }

    int semid = semget(sem_key, 2, IPC_CREAT | 0666); // [0] - buff is full, [1] - is free
    if (semid == -1) {
        perror("semget");
        shmdt(shm_ptr);
        close(fd_input_file);
        if (fd_output_file != 1)
            close(fd_output_file);
        return 1;
    }

    if (semctl(semid, 0, SETVAL, 1) == -1) { // sem_empty = 1
        perror("semctl SETVAL 0");
        shmdt(shm_ptr);
        close(fd_input_file);
        if (fd_output_file != 1)
            close(fd_output_file);
        return 1;
    }

    if (semctl(semid, 1, SETVAL, 0) == -1) { // sem_full = 0
        perror("semctl SETVAL 1");
        shmdt(shm_ptr);
        close(fd_input_file);
        if (fd_output_file != 1)
            close(fd_output_file);
        return 1;
    }
    
    struct timespec start, end;

    pid_t pid = fork();
    if (pid < 0) {

        DEBUG_PRINTF("fork failed!\n");
        return -1;
    }

    if (pid == 0) {

        while (1) {

            if(!sem_w8_(semid, 1)) {

                shmdt(shm_ptr);
                exit(1);
            }

            if(shm_ptr->eof_flag) {

                if(!sem_post_(semid, 0)) {

                    shmdt(shm_ptr);
                    exit(1);
                }
                break;
            }
            
            size_t bytes_written_total = 0;
            size_t bytes_to_write = shm_ptr->buf_size;
            const char *buf_ptr = shm_ptr->buffer;

            while (bytes_written_total < bytes_to_write) {
                ssize_t curr_size = write(fd_output_file, buf_ptr + bytes_written_total, bytes_to_write - bytes_written_total);

                if (curr_size == -1) {

                    perror("write");
                    shmdt(shm_ptr);
                    exit(1);
                }

                bytes_written_total += curr_size;
            }

            if(!sem_post_(semid, 0)) {

                shmdt(shm_ptr);
                exit(1);
            }
        }

        shmdt(shm_ptr);
        exit(0);
    }

    else {

        int64_t curr_size = 0;
        clock_gettime(CLOCK_MONOTONIC, &start);
        while (1) {
            
            if (!sem_w8_(semid, 0)) 
                break;

            curr_size = read(fd_input_file, shm_ptr->buffer, BUF_SIZE);

            if (curr_size > 0) {

                shm_ptr->buf_size = curr_size;
                sem_post_(semid, 1); 
            } 

            else if (curr_size == 0) {

                shm_ptr->eof_flag = 1;
                sem_post_(semid, 1);  
                break;
            } 
            
            else {

                sem_post_(semid, 0);  
                break;
            }
        }

        if (curr_size == 0) { 
            
            if (!sem_w8_(semid, 0)) {

                perror("sem_w8_ EOF wait empty");
            }
            
            else {

                shm_ptr->eof_flag = 1; 

                if (!sem_post_(semid, 1)) 
                    perror("sem_post_ EOF signal full");
            }
        } 

        else if (curr_size == -1) {

            perror("read");
        }

        close(fd_input_file);
        if (fd_output_file != 1)
            close(fd_output_file);
        wait(NULL);
        clock_gettime(CLOCK_MONOTONIC, &end);
        double time_taken = (end.tv_sec - start.tv_sec) + 
                            (end.tv_nsec - start.tv_nsec) / 1e9;
        fprintf(stderr, "%s: Time consumed = %.6f seconds\n", input_filename, time_taken);
        *time_consumed = time_taken;
        shmdt(shm_ptr);
        shmctl(shmid, IPC_RMID, NULL);
        semctl(semid, 0, IPC_RMID, NULL);
    }

    return 0;
}

void generate_gnuplot_script(double fifo_times[3], double mq_times[3], double shm_times[3]) {
    char buffer[256];
    int fd;

    const char* sizes[] = {"8196", "4194304", "4294967296"}; // 8196 байт, 4 Мб, 4 Гб
    const char* labels[] = {"8196 байт", "4 Мб", "4 Гб"};
    const char* methods[] = {"FIFO", "SYS V MQ", "SYS V SHM"};
    
    for (int i = 0; i < 3; i++) {
        // Создаем файл с данными для текущего размера
        char data_filename[64];
        snprintf(data_filename, sizeof(data_filename), "times_%d.txt", i);
        
        fd = open(data_filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd != -1) {
            // Заголовок
            const char* header = "Method\tTime\n";
            write(fd, header, strlen(header));
            
            // Данные для каждого метода
            double times[] = {fifo_times[i], mq_times[i], shm_times[i]};
            
            for (int j = 0; j < 3; j++) {
                int len = snprintf(buffer, sizeof(buffer), 
                    "\"%s\"\t%.6f\n", 
                    methods[j], times[j]);
                write(fd, buffer, len);
            }
            close(fd);
        }
        
        // Создаем gnuplot скрипт для текущего размера
        char script_filename[64];
        snprintf(script_filename, sizeof(script_filename), "plot_%d.gnu", i);
        
        fd = open(script_filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd != -1) {
            // Генерируем gnuplot скрипт с правильными значениями
            char gnuplot_script[1024];
            snprintf(gnuplot_script, sizeof(gnuplot_script), GNUPLOT_HEADER, labels[i], sizes[i], data_filename);
            
            write(fd, gnuplot_script, strlen(gnuplot_script));
            close(fd);
        }
    }

    printf("Файлы данных и скрипты сгенерированы.\n");
    printf("Запустите: gnuplot plot_0.gnu && gnuplot plot_1.gnu && gnuplot plot_2.gnu\n");
    printf("PNG файлы будут созданы: plot_8196.png, plot_4194304.png, plot_4294967296.png\n");
}