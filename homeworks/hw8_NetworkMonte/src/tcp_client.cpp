#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>
#include "Client.hpp"
#include "cliver.hpp"

volatile sig_atomic_t running = 1;

void handle_signal(int sig, siginfo_t* info, void* context);
int broadcast_search();

void handle_signal(int sig, siginfo_t* info, void* context) {
    running = 0;
    printf("\nКлиент завершает работу...\n");
    return;
}

int broadcast_search() {
    int sockfd = 0;
    struct sockaddr_in broadcast_addr;
    char buffer[MAX_BUFFER] = "BROADCAST_SEARCH";
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

    printf("Отправляю широковещательный запрос...\n");

    // Отправляем широковещательный запрос
    if (sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr)) < 0) {
        perror("sendto");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Ожидаю ответы от серверов...\n");

    // Принимаем ответы
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);
    int servers_found = 0;

    while (1) {
        memset(buffer, 0, MAX_BUFFER);
        int n = (int)recvfrom(sockfd, buffer, MAX_BUFFER - 1, 0, (struct sockaddr*)&server_addr, &addr_len);
        if (n < 0) {
            // Таймаут
            break;
        }

        buffer[n] = '\0';
        char* server_ip = inet_ntoa(server_addr.sin_addr);
        int server_port = ntohs(server_addr.sin_port);

        printf("Найден сервер: %s:%d - %s\n", server_ip, server_port, buffer);
        servers_found++;
    }

    if (servers_found == 0) {
        printf("Серверы не найдены\n");
	exit(EXIT_FAILURE);
    }

    printf("Найдено серверов: %d\n", servers_found);
    close(sockfd);
    return 0;
}

int main() {
    struct sigaction sa;
    sa.sa_sigaction = handle_signal;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTSTP, &sa, NULL);

    if (broadcast_search() == -1)
	return 0;

    int sockfd = 0;
    struct sockaddr_in server_addr;
    char buffer[MAX_BUFFER] = {};

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Настройка адреса сервера
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // Преобразуем IP-адрес
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Подключение к серверу
    printf("Подключение к серверу %s:%d...\n", SERVER_IP, PORT);
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Успешно подключен к серверу!\n");
    printf("Вводите строки (для выхода введите 'exit' или 'quit'):\n");

    // Установка таймаута на получение
    struct timeval tv;
    tv.tv_sec = RECEIVE_TIMEOUT;  // 5 секунд таймаут
    tv.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1) {
	perror("setsockopt");
    }

    while (running) {
        printf("> ");
        fflush(stdout);

	if (fgets(buffer, MAX_BUFFER, stdin) == NULL) {
	    if (errno == EINTR)
		printf("Сервер отключился\n");
	    else
		perror("fgets");
	    break;
	}

        // Убираем символ новой строки
        buffer[strcspn(buffer, "\n")] = '\0';

        // Проверяем команды выхода
        if (strcmp(buffer, "exit") == 0 || strcmp(buffer, "quit") == 0) {
            printf("Отключение от сервера...\n");
            break;
        }

        // Проверяем пустую строку
        if (strlen(buffer) == 0) {
            printf("Введите непустую строку\n");
            continue;
        }

        // Отправляем строку серверу
        strcat(buffer, "\n");  // Добавляем символ новой строки
        if (send(sockfd, buffer, strlen(buffer), 0) == -1) {
            perror("send");
            break;
        }

        // Получаем ответ от сервера
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = (int)recv(sockfd, buffer, MAX_BUFFER - 1, 0);
        if (bytes_received == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                printf("Таймаут: сервер не ответил\n"); 
	    else
                perror("recv");
            break;
        } 
	if (bytes_received == 0) {
            printf("Сервер отключился\n");
            break;
        }
 
        buffer[bytes_received] = '\0';
        printf("Сервер: %s", buffer);
    }

    printf("Закрытие соединения...\n");
    close(sockfd);

    return 0;
}