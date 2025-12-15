#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <time.h>
#include <sys/wait.h>
#include <math.h>
#include "cliver.hpp"
#include "Server.hpp"

volatile sig_atomic_t running = 1;

typedef double(*func_t)(double); 

struct thread_data_t {
    int thread_id;
    int points_per_thread;
    func_t func;
    double a;
    double b;
    double y_max;
    int shm_id;
    double* results;
};

void *thread_calculate(void* args);
double f(double x);
void handle_signal(int sig, siginfo_t* info, void* context);
void handle_client(int client_fd, struct sockaddr_in *client_addr);
int broadcast_response();
int find_free_tcp_port(int start_port, int end_port);
double calculateMonteCarlo(double a, double b);

void handle_signal(int sig, siginfo_t* info, void* context) {
    running = 0;
    printf("\n>> Сервер завершает работу\n");
    return;
}

void *thread_calculate(void *args) {
    if (!args) {
	perror("Null pointer");
	exit(EXIT_FAILURE);
    }

    thread_data_t* thread_data = (thread_data_t*)args;
    int points_count = 0;

    unsigned int seed = (unsigned int)(time(NULL) ^ thread_data->thread_id);

    for (int i = 0; i < thread_data->points_per_thread; i++) {
        double x = thread_data->a + ((double)rand_r(&seed) / RAND_MAX) * (thread_data->b - thread_data->a);
	double y = ((double)rand_r(&seed) / RAND_MAX) * thread_data->y_max;

	if (fabs(y) - fabs(thread_data->func(x)) <= 0)
	    points_count++;
    }

    thread_data->results[thread_data->thread_id] = (double)points_count / thread_data->points_per_thread * 
			    (thread_data->b - thread_data->a) * thread_data->y_max;

    pthread_exit(NULL);
}

double f(double x) {
    return x * x;
}

double calculateMonteCarlo(double a, double b) {
    struct timespec start, end;
    double transmission_time = 0;

    int num_threads = (int)sysconf(_SC_NPROCESSORS_CONF);
    double total = 0;

    key_t key = 0;
    if ((key = ftok("/tmp", 'A')) == -1) {
        perror("ftok");
	exit(EXIT_FAILURE);
    }

    int shmid = 0;
    if ((shmid = shmget(key, (long unsigned int)num_threads * sizeof(double), 0666 | IPC_CREAT)) == -1) {
        perror("shmget");
	exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid < 0) {
	perror("fork");
	exit(EXIT_FAILURE);
    }
    if (pid == 0) {
	double* shared_results = (double*)shmat(shmid, NULL, 0);
	if (shared_results == (void*)-1) {
	    perror("shmat");
	    exit(EXIT_FAILURE);
	}

	pthread_t* threads = (pthread_t*)calloc((size_t)num_threads, sizeof(pthread_t));
	thread_data_t* thread_data = (thread_data_t*)calloc((size_t)num_threads, sizeof(thread_data_t));
	
	for (int i = 0; i < num_threads; i++) {
	    double cur_a = a + (b - a) / num_threads * i;
	    double cur_b = a + (b - a) / num_threads * (i + 1);
	    double cur_A = f(cur_a);
	    double cur_B = f(cur_b);
	    thread_data[i] = {.thread_id = i, .points_per_thread = (int)(NUM_POINTS / num_threads), .func = f, .a = cur_a, 
				.b = cur_b, .y_max = (cur_A > cur_B ? cur_A : cur_B), .results = shared_results};
	    pthread_create(&threads[i], NULL, thread_calculate, (void*)(&thread_data[i]));
	}
	
	for (int i = 0; i < num_threads; i++)
	    pthread_join(threads[i], NULL);
	
	shmdt(shared_results);
	exit(EXIT_SUCCESS);
    }
    else {
	double* shared_results = (double*)shmat(shmid, NULL, 0);
	if (shared_results == (void*)-1) {
	    perror("shmat");
	    exit(EXIT_FAILURE);
	}

	if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
	    perror("clock_gettime");
	    exit(EXIT_FAILURE);
	}

	wait(NULL);

	if (clock_gettime(CLOCK_MONOTONIC, &end) == -1) {
	    perror("clock_gettime");
	    exit(EXIT_FAILURE);
	}

	for (int i = 0; i < num_threads; i++) 
	    total += shared_results[i];

	shmdt(shared_results);
    }

    transmission_time = (double)(end.tv_sec - start.tv_sec) + (double)(end.tv_nsec - start.tv_nsec) / (double)BILLION;
    printf(">> Количество потоков: %d, общее время вычислений: %lf\n", num_threads, transmission_time);

    shmctl(shmid, IPC_RMID, 0);
    return total;
}

int find_free_tcp_port(int start_port, int end_port) {
    int sockfd = 0;
    struct sockaddr_in addr;

    for (int port = start_port; port <= end_port; port++) {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1) {
            perror("socket FIND_PORT");
            continue;
        }

        // Устанавливаем SO_REUSEADDR для быстрого переиспользования порта
        int reuse = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons((uint16_t)port);

        if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
            close(sockfd);  // Закрываем тестовый сокет
            return port;  // Нашли свободный порт
        }

        close(sockfd);
    }

    return -1;  // Не нашли свободный порт
}

void handle_client(int client_fd, struct sockaddr_in *client_addr) {
    char buffer[MAX_BUFFER] = {};
    int bytes_received = 0;
    char client_ip[INET_ADDRSTRLEN] = {};

    // Преобразуем IP-адрес в строку
    inet_ntop(AF_INET, &(client_addr->sin_addr), client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client_addr->sin_port);

    printf(">> Клиент подключен: %s:%d\n", client_ip, client_port);

    while (running) {
        memset(buffer, 0, sizeof(buffer));

        bytes_received = (int)recv(client_fd, buffer, MAX_BUFFER - 1, 0);
        if (bytes_received == 0) {
	    printf(">> Клиент %s:%d отключился\n", client_ip, client_port);
	    break;
        }

	if (bytes_received == -1) {
	    if (errno == EINTR)
		printf(">> Остановка сервера сигналом\n");
	    else 
		perror("recv FROM_CLIENT");
	    break;
	}

        // Завершаем строку
        buffer[bytes_received] = '\0';

        // Убираем символ новой строки, если есть
        buffer[strcspn(buffer, "\n")] = '\0';
        buffer[strcspn(buffer, "\r")] = '\0';

        printf(">> От клиента %s:%d: \"%s\"\n", client_ip, client_port, buffer);
	double a = 0, b = 0;
	sscanf(buffer, "%lg %lg", &a, &b);

	double result = calculateMonteCarlo(a, b);

        // Подготавливаем ответ
        char response[MAX_BUFFER] = {};
        int response_len = snprintf(response, sizeof(response), "%lg\n", result);

        if (send(client_fd, response, (size_t)response_len, 0) == -1) {
            perror("send TO_CLIENT");
            break;
        }
    }

    close(client_fd);
    printf(">> Соединение с клиентом %s:%d закрыто\n", client_ip, client_port);
    return;
}

int broadcast_response() {
    int sockfd = 0;
    struct sockaddr_in server_addr, client_addr;
    char buffer[MAX_BUFFER] = {};
    socklen_t addr_len = sizeof(client_addr);
    int broadcast_enable = 1;
    int free_port = 0;

    // Создаем UDP сокет
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket UDP");
        exit(EXIT_FAILURE);
    }

    // Включаем опцию широковещания (для ответа)
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable)) < 0) {
        perror("setsockopt BROADCAST");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Привязываем сокет к порту
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(BROADCAST_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind UDP");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf(">> Сервер запущен. Ожидание запросов на порту %d\n", BROADCAST_PORT);

    while (running) {
        memset(buffer, 0, MAX_BUFFER);

        // Получаем запрос
        int received_bytes = (int)recvfrom(sockfd, buffer, MAX_BUFFER - 1, 0, (struct sockaddr*)&client_addr, &addr_len);
        if (received_bytes == -1) {
	    if (errno == EINTR) {
		printf(">> Завершение BROADCAST сигналом\n");
		close(sockfd);
		return -1;
	    }
            perror("recvfrom");
            continue;
        }

        buffer[received_bytes] = '\0';
        char* client_ip = inet_ntoa(client_addr.sin_addr);
        int client_port = ntohs(client_addr.sin_port);

        printf(">> Получен запрос от %s:%d: %s\n", client_ip, client_port, buffer);

        // Проверяем, это ли наш запрос
        if (strcmp(buffer, BROADCAST_MSG) == 0) {
            char response[MAX_BUFFER] = {};
	    free_port = find_free_tcp_port(8000, 9000);
	    long cores = sysconf(_SC_NPROCESSORS_CONF);
	    snprintf(response, MAX_BUFFER, "%d %ld", free_port, cores);

            // Отправляем ответ клиенту
            if (sendto(sockfd, response, strlen(response), 0, (struct sockaddr*)&client_addr, addr_len) == -1)
                perror("sendto CLIENT");

	    printf(">> Отправлен ответ клиенту %s:%d\n", client_ip, client_port);
	    break;
        }
    }

    close(sockfd);
    return free_port;
}

int main() {
    struct sigaction sa;
    sa.sa_sigaction = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTSTP, &sa, NULL);

    int server_port = 0;
    if ((server_port = broadcast_response()) == -1)
	return 0;

    int server_fd = 0, client_fd = 0;
    struct sockaddr_in server_addr, client_addr;
    size_t client_len = 0;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket TCP");
        exit(EXIT_FAILURE);
    }

    // Настройка опций сокета (для повторного использования адреса)
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt SO_REUSEADDR");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Настройка адреса сервера
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons((uint16_t)server_port);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind TCP");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, BACKLOG) == -1) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf(">> Запущен сервер на порту: %d\n", server_port); 
    printf(">> Ожидание подключений\n");

    while (running) {
        client_len = sizeof(client_addr);

        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, (socklen_t*)&client_len);
        if (client_fd == -1) {
	    if (running)
		perror("accept");
	    break;
        }

        handle_client(client_fd, &client_addr);
    }

    printf(">> Сервер закрыт\n");
    close(server_fd);

    return 0;
}