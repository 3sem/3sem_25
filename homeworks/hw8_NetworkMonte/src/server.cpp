#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>
#include "cliver.hpp"
#include "Server.hpp"

volatile sig_atomic_t running = 1;

void handle_signal(int sig, siginfo_t* info, void* context);
void handle_client(int client_fd, struct sockaddr_in *client_addr);
int broadcast_response();
int find_free_tcp_port(int start_port, int end_port);

void handle_signal(int sig, siginfo_t* info, void* context) {
    running = 0;
    printf("\n>> Сервер завершает работу\n");
    return;
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

        // Подготавливаем ответ
        char response[MAX_BUFFER] = {};
        int response_len = snprintf(response, sizeof(response), "Сумма: %lg\n", a + b);

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