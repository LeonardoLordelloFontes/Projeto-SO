#include <sys/types.h>
#include <unistd.h> /* chamadas ao sistema: defs e decls essenciais */
#include <fcntl.h> /* O_RDONLY, O_WRONLY, O_CREAT, O_* */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include "queue.h"
#include "server.h"

int server_transformations[7][2];

/* Perfect Hash
    nop - 4
    bcompress - 1
    bdecompress - 2
    gcompress - 6
    gdecompress - 0
    encrypt - 3
    decrypt - 5
*/

int get_transformation_key(char* str) {
    int key = (str[0] + str[1]) % 7;
    if (str[0] == 'e') 
        key += 2;
    return key;
}

int get_task_priority(const char *request) {
    int priority = 0;
    char buffer[strlen(request) + 1];
    strcpy(buffer, request);
    char *token = strtok(buffer, " ");
    token = strtok(NULL, " ");
    if (strcmp(token, "-p") == 0) {
        token = strtok(NULL, " ");
        priority = atoi(token);
        if (priority < 0) priority = 0;
        else if (priority > 5) priority = 5;
    }
    return priority;
}

int check_transformations_availableness(int user_transformations[7]) {
    int available = 1;
    for (int i = 0; i < 7 && available; i++) {
        if (server_transformations[i][0] + user_transformations[i] > server_transformations[i][1])
            available = 0;
    }
    return available;
}

int accept_user_request(int user_transformations[7]) {
    int accepted = 1;
    for (int  i = 0; i < 7 && accepted; i++) {
        if (user_transformations[i] > server_transformations[i][1])
            accepted = 0;
    }
    return accepted;
}

int skip_request_to_transformations(char *request) {
    char buffer[strlen(request) + 1];
    strcpy(buffer, request);
    int skip = 0;
    char *token = strtok(buffer, " "); // proc-file
    skip += strlen(token) + 1;
    token = strtok(NULL, " "); // -p ou output
    skip += strlen(token) + 1;
    if (strcmp(token, "-p") == 0) {
        token = strtok(NULL, " "); // priority
        skip += strlen(token) + 1;
        token = strtok(NULL, " "); // output
        skip += strlen(token) + 1;
    }
    token = strtok(NULL, " "); // input
    skip += strlen(token) + 1;
    return skip;
}

void fill_user_transformations(char *request, int user_transformations[7]) {
    int skip = skip_request_to_transformations(request);
    for (char *token = strtok(request + skip, " "); token != NULL; token = strtok(NULL, " ")) {
        user_transformations[get_transformation_key(token)]++;
    }
}

void status_task() {
    // TODO
}

void process_transformations(char *transformations) {
    pid_t pid = fork();
    if (pid == 0) {
        // TODO
        _exit(1);
    }
}

void proc_file_task(char *request, int request_id, char *request_pid) {
    Task task = {.transformations = {0}};
    fill_user_transformations(request, task.transformations);
    int client_server_fd = open(request_pid, O_WRONLY);
    if (client_server_fd == -1) {
        perror("open");
        return 1;
    }
    if (accept_user_request(task.transformations)) {
        write(client_server_fd, "PENDING", 7);
        close(client_server_fd);
        if (isEmpty()) {
            int skip = skip_request_to_transformations(request);
            process_transformations(request + skip);
        }
        else {
            task.priority = get_task_priority(request);
            task.request_id = request_id;
            strcpy(task.request_pid, request_pid);
            strcpy(task.request, request);
            enqueue(task);
        }
    }
    else {
        // PEDIDO RECUSADO
        // ENVIAR MENSAGEM AO CLIENTE DIZENDO "DENIED"
        write(1, "NO", 2); 
    }

    // olhar para o topo e dar dequeue enquanto estiver disponivel 
}

void select_task(char *request, int request_id) {
    char buffer[strlen(request) + 1]; 
    strcpy(buffer, request);
    char *token = strtok(buffer, " ");
    char request_pid[32];
    strcpy(request_pid, token);
    int skip_pid = strlen(token) + 1;
    token = strtok(NULL, " ");

    if (strcmp(token, "proc-file") == 0) {
        proc_file_task(request + skip_pid, request_id, request_pid);
    }

    else if (strcmp(token, "status") == 0) {
        // TODO
        status_task();
    }
}

// testado

void max_runnable_transformations(char* config_path) {
    int fd = open(config_path, O_RDONLY);
    if (fd == -1) {
        perror("open");
        _exit(1);
    }
    char buffer[128];
    int n = read(fd, buffer, 128);
    if (n == -1) {
        perror("read");
        _exit(1);
    } 
    close(fd);
    for (char *token = strtok(buffer, " \n"); token != NULL; token = strtok(NULL, " \n")) {
        int key = get_transformation_key(token);
        token = strtok(NULL, " \n");
        server_transformations[key][0] = 0; // running 
        server_transformations[key][1] = atoi(token); // max
    }
}

ssize_t read_request(int fd, char* request, size_t size) {
    ssize_t bytes_read = read(fd, request, size);
    size_t request_size = strcspn(request, "\n");
    request[request_size] = '\0';
    lseek(fd, request_size - bytes_read, SEEK_CUR);
    return request_size;
}

int main(int argc, char *argv[]) {
    max_runnable_transformations(argv[1]);
    mkfifo("server_client_fifo", 0666);
    initQueue();
    char request[256];
    int requests = 0;
    while (1) {
        int fd = open("server_client_fifo", O_RDONLY);
        int n = read_request(fd, request, 256);
        close(fd);
        requests++;
        select_task(request, requests);
    }
    return 0;
}