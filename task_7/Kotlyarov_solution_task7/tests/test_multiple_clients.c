#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <errno.h>

#define NUM_CLIENTS 10
#define FILES_PER_CLIENT 5

void create_test_file(const char* filename, int version) {
    FILE* file = fopen(filename, "w");
    if (file) {
        for (int j = 0; j < 5; j++) {
            fprintf(file, "Line %d: Test file v%d\n", j, version);
        }
        fclose(file);
    }
}

void run_client(int client_id) {
    char command[2048];
    // Используем правильный путь к клиенту
    snprintf(command, sizeof(command), "./bin/client");
    
    for (int i = 0; i < FILES_PER_CLIENT; i++) {
        char file_arg[256];
        // Используем правильные пути к тестовым файлам
        snprintf(file_arg, sizeof(file_arg), " -r ../tests/test_file%d.txt", (i % 3) + 1);
        strcat(command, file_arg);
    }
    
    char output_file[256];
    // Используем правильный путь для выходных файлов
    snprintf(output_file, sizeof(output_file), " -w ../tests/client_%d_output.txt", client_id);
    strcat(command, output_file);
    
    printf("Starting client %d with command: %s\n", client_id, command);
    int result = system(command);
    if (result != 0) {
        printf("Client %d failed: %d\n", client_id, result);
    } else {
        printf("Client %d completed successfully\n", client_id);
    }
}

int main() {
    printf("=== Starting IO Server Test ===\n");
    
    // Создаем директорию для тестовых файлов, если её нет
    struct stat st = {0};
    if (stat("../tests", &st) == -1) {
        mkdir("../tests", 0755);
    }
    
    // Запускаем сервер
    pid_t server_pid = fork();
    if (server_pid == 0) {
        // Дочерний процесс - сервер
        printf("Starting server...\n");
        execl("./bin/server", "./bin/server", NULL);
        perror("Failed to start server");
        exit(1);
    } else if (server_pid < 0) {
        perror("Failed to fork server");
        return 1;
    }
    
    printf("Server started with PID: %d\n", server_pid);
    sleep(2); // Даем серверу время на запуск
    
    // Создаем тестовые файлы
    printf("Creating test files...\n");
    for (int i = 0; i < 3; i++) {
        char filename[256];
        snprintf(filename, sizeof(filename), "../tests/test_file%d.txt", i + 1);
        create_test_file(filename, i + 1);
        printf("Created test file: %s\n", filename);
    }
    
    // Запускаем клиентов
    printf("Starting %d clients...\n", NUM_CLIENTS);
    int success = 0;
    for (int i = 0; i < NUM_CLIENTS; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            run_client(i + 1);
            exit(0);
        } else if (pid > 0) {
            success++;
        } else {
            perror("Failed to fork client");
        }
    }
    
    // Ждем завершения клиентов
    printf("Waiting for clients to complete...\n");
    for (int i = 0; i < NUM_CLIENTS; i++) {
        wait(NULL);
    }
    
    printf("All clients completed\n");
    
    // Останавливаем сервер
    printf("Stopping server...\n");
    kill(server_pid, SIGTERM);
    waitpid(server_pid, NULL, 0);
    
    // Чистка
    printf("Cleaning up test files...\n");
    for (int i = 0; i < 3; i++) {
        char filename[256];
        snprintf(filename, sizeof(filename), "../tests/test_file%d.txt", i + 1);
        remove(filename);
        printf("Removed test file: %s\n", filename);
    }
    
    printf("=== Test completed ===\n");
    return 0;
}
