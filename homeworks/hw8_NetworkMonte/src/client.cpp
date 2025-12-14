#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>
#include <math.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include "Client.hpp"
#include "cliver.hpp"

struct ServerInfo_t {
    char* ip;
    int port;
    int cores;
};

volatile sig_atomic_t running = 1;

void handle_signal(int sig, siginfo_t* info, void* context);
int broadcast_search(struct ServerInfo_t* servers);

void handle_signal(int sig, siginfo_t* info, void* context) {
    running = 0;
    printf("\n>> Клиент завершает работу сигналом\n");
    return;
}

int broadcast_search(struct ServerInfo_t* servers) {
    int sockfd = 0;
    struct sockaddr_in broadcast_addr;
    char buffer[MAX_BUFFER] = BROADCAST_MSG;
    int broadcast_enable = 1;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
	perror("socket UDP");
	exit(EXIT_FAILURE);
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable)) == -1) {
	perror("setsockopt BROADCAST");
	close(sockfd);
	exit(EXIT_FAILURE);
    }

    struct timeval timeout;
    timeout.tv_sec = RECEIVE_TIMEOUT;
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
	perror("setsockopt TIMEOUT");
	close(sockfd);
	exit(EXIT_FAILURE);
    }

    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(BROADCAST_PORT);
    broadcast_addr.sin_addr.s_addr = inet_addr(BROADCAST_IP);

    printf(">> Отправляю широковещательный запрос\n");

    // Отправляем широковещательный запрос
    if (sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr)) < 0) {
        perror("sendto BROADCAST");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf(">> Ожидаю ответы от серверов\n");

    // Принимаем ответы
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);
    int servers_found = 0;

    while (running) {
        memset(buffer, 0, MAX_BUFFER);
        int received_bytes = (int)recvfrom(sockfd, buffer, MAX_BUFFER - 1, 0, (struct sockaddr*)&server_addr, &addr_len);
        if (received_bytes == -1) {
            if (errno == EINTR) {
		printf(">> Завершение BROADCAST сигналом\n");
		close(sockfd);
		return -1;
	    }
            break;
        }

        buffer[received_bytes] = '\0';
        sscanf(buffer, "%d %d", &servers[servers_found].port, &servers[servers_found].cores);
	servers[servers_found].ip = inet_ntoa(server_addr.sin_addr);

        printf(">> Найден сервер: %s:%d - port: %d, cores: %d\n", servers[servers_found].ip,
		BROADCAST_PORT, servers[servers_found].port, servers[servers_found].cores);
        servers_found++;
    }

    if (servers_found == 0) {
        printf(">> Серверы не найдены\n");
	return -1;
    }

    printf(">> Найдено серверов: %d\n", servers_found);
    close(sockfd);
    return servers_found;
}

int main() {
    struct sigaction sa;
    sa.sa_sigaction = handle_signal;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTSTP, &sa, NULL);

    struct ServerInfo_t servers[MAX_SERVERS] = {};
    int serv_count = 0;
    if ((serv_count = broadcast_search(servers)) == -1)
	return 0;

    int overall_cores = 0;
    for (int i = 0; i < serv_count; i++)
	overall_cores += servers[i].cores;

    int* sockfd = (int*)calloc((size_t)serv_count, sizeof(int));
    if (!sockfd) {
	perror("calloc");
	return 0;
    }

    struct sockaddr_in* server_addr = (struct sockaddr_in*)calloc((size_t)serv_count, sizeof(struct sockaddr_in));
    if (!server_addr) {
	perror("calloc");
	return 0;
    }

    struct timeval tv;
    tv.tv_sec = RECEIVE_TIMEOUT;
    tv.tv_usec = 0;

    for (int i = 0; i < serv_count; i++) {
	if ((sockfd[i] = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	    perror("socket TCP");
	    exit(EXIT_FAILURE);
	}

	memset(&server_addr[i], 0, sizeof(server_addr[0]));
	server_addr[i].sin_family = AF_INET;
	server_addr[i].sin_port = htons((uint16_t)servers[i].port);

	if (inet_pton(AF_INET, servers[i].ip, &server_addr[i].sin_addr) == -1) {
	    perror("inet_pton SERVER");
	    close(sockfd[i]);
	    continue;
	}

	printf(">> Подключение к серверу %s:%d\n", servers[i].ip, servers[i].port);
	if (connect(sockfd[i], (struct sockaddr*)&server_addr[i], sizeof(server_addr[0])) == -1) {
	    perror("connect TO_SERVER");
	    close(sockfd[i]);
	    continue;
	}

	printf(">> Успешно подключен к серверу %s:%d!\n", servers[i].ip, servers[i].port);

	if (setsockopt(sockfd[i], SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1) {
	    perror("setsockopt TIMEOUT");
	}
    }

    while (running) {
	fflush(stdout);

	double a = 0, b = 0;
	printf("> Введите параметры интегрирования (A b): ");
	if (scanf("%lg %lg", &a, &b) < 2) {
	    if (errno == EINTR)
		printf(">> Остановка клиента сигналом\n");
	    else
		perror("scanf");
	    break;
	}

	char buffer[MAX_BUFFER] = {};

	for (int i = 0; i < serv_count; i++) {
	    memset(buffer, 0, sizeof(buffer));
	    double cur_a = a + (b - a) / overall_cores * servers[i].cores * i;
	    double cur_b = a + (b - a) / overall_cores * servers[i].cores * (i + 1);
	    snprintf(buffer, MAX_BUFFER, "%lg %lg", cur_a, cur_b);

	    if (send(sockfd[i], buffer, strlen(buffer), 0) == -1) {
		perror("send TO_SERVER");
		break;
	    }
	}

	for (int i = 0; i < serv_count; i++) {
	    memset(buffer, 0, sizeof(buffer));
	    int bytes_received = (int)recv(sockfd[i], buffer, MAX_BUFFER - 1, 0);
	    if (bytes_received == -1) {
		if (errno == EAGAIN || errno == EWOULDBLOCK)
		    printf(">> Таймаут: сервер %s:%d не ответил\n", servers[i].ip, servers[i].port); 
		else
		    perror("recv FROM_SERVER");
		break;
	    }
	    if (bytes_received == 0) {
		printf(">> Сервер %s:%d отключился\n", servers[i].ip, servers[i].port);
		break;
	    }

	    buffer[bytes_received] = '\0';
	    printf(">> Сервер %s:%d: %s", servers[i].ip, servers[i].port, buffer);
	}
    }

    printf(">> Закрытие соединения\n");
    if (sockfd) {
	free(sockfd);
	sockfd = NULL;
    }

    if (server_addr) {
	free(server_addr);
	server_addr = NULL;
    }

    return 0;
}