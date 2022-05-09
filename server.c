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
RunningTasks running_tasks;

/* Perfect Hash
    nop - 4
    bcompress - 1
    bdecompress - 2
    gcompress - 6
    gdecompress - 0
    encrypt - 3
    decrypt - 5
*/

void add_running_task(Task task) {
    RunningTasks new_node = malloc(sizeof(struct RunningTask));
    new_node->task = task;
    if (running_tasks == NULL) {
        new_node->next = NULL;
        running_tasks = new_node;
    }
    else {
        new_node->next = running_tasks;
        running_tasks = new_node;
    }
}

void remove_running_task(char *pid) {
    RunningTasks aux;
    if (pid == running_tasks->task.request_pid) {
        aux = running_tasks;
        running_tasks = running_tasks->next;
        free(aux);
    }
    else {
        RunningTasks last;
        for (aux = running_tasks; aux != NULL && aux->task.request_pid != pid; aux = aux->next)
            last = aux;
        if (aux == NULL) {
            aux = last;
            last = NULL;
            free(aux);
        }
        else {
            last->next = aux->next;
            free(aux);
        }
    }
}

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

int check_transformations_availableness(int* user_transformations) {
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

int skip_request_priority(char *request) {
    char buffer[strlen(request) + 1];
    strcpy(buffer, request);
    int skip = 0;
    char *token = strtok(buffer, " "); // proc-file
    skip += strlen(token) + 1;
    token = strtok(NULL, " "); // -p ou input
    if (strcmp(token, "-p") == 0) {
        skip += strlen(token) + 1;
        token = strtok(NULL, " "); // priority
        skip += strlen(token) + 1;
    }
    return skip;
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
    char buffer[strlen(request)];
    int skip = skip_request_to_transformations(request);
    strcpy(buffer, request + skip);
    for (char *token = strtok(buffer, " "); token != NULL; token = strtok(NULL, " ")) {
        user_transformations[get_transformation_key(token)]++;
    }
}

void status_task() {
    for (RunningTasks aux = running_tasks; aux != NULL; aux = aux->next) {
        char buffer[512];
        snprintf(buffer, 512, "task #%d: %s\n", aux->task.request_id, aux->task.request);
        write(1, buffer, strlen(buffer));
    }
}

int get_number_of_transformations(int user_transformations[7]) {
    int transformations = 0;
    for (int i = 0; i < 7; i++) {
        transformations += user_transformations[i];
    }
    return transformations;
}

void process_transformations(char* request, char* request_pid, char* transformations_path, int number_of_transformations) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
    }
    if (pid == 0) {
        // write(1, request, strlen(request));
        int pipes[number_of_transformations][2];
        char in[128], out[128];
        char *token = strtok(request, " ");
        strncpy(in, token, 128);
        token = strtok(NULL, " ");
        strncpy(out, token, 128);
        
        for (int i = 0; i < number_of_transformations; i++) {
            token = strtok(NULL, " ");
            // write(1, token, strlen(token));
            int in_fd;
            int out_fd;
            if (i == 0) {
                in_fd = open(in, O_RDONLY);
                if (in_fd == -1) {
                    write(1, "NO", 2);
                    // alterar depois para n terminar o servidor
                    // enviar mensagem ao usuário denied?
                    perror("open");
                    _exit(1);
                }
            }
            else 
                in_fd = pipes[i-1][0];
            if (i == number_of_transformations - 1) {
                out_fd = open(out, O_CREAT|O_TRUNC|O_WRONLY, 0666);
                if (out_fd == -1) {
                    write(1, "NO", 2);
                    perror("open");
                    _exit(1);
                }
            }
            else {
                pipe(pipes[i]);
                out_fd = pipes[i][1];
            }
            pid_t pid_transformation = fork();
            if (pid_transformation == 0) {
                dup2(in_fd, 0);
                dup2(out_fd, 1);
                close(in_fd);
                close(out_fd);
                char exec_transformation_path[256];
                snprintf(exec_transformation_path, 256, "./%s/%s", transformations_path, token);
                execlp(exec_transformation_path, exec_transformation_path, NULL);
                perror("exec");
                _exit(1);
            }
            else {
                wait(NULL);
                close(out_fd);
                close(in_fd);
            }
        }

        int client_server_fd = open(request_pid, O_WRONLY);
        write(client_server_fd, "concluded\n", 11);
        close(client_server_fd);
        _exit(0);
    }
}

void proc_file_task(char *request, int request_id, char *request_pid, char* transformations_path) {
    Task task = {.transformations = {0}};
    fill_user_transformations(request, task.transformations);
    /* int client_server_fd = open(request_pid, O_WRONLY);
    if (client_server_fd == -1) {
        perror("open");
        _exit(1);
    }*/
    if (accept_user_request(task.transformations)) {
        // write(client_server_fd, "pending\n", 9);
        task.priority = get_task_priority(request);
        task.request_id = request_id;
        strcpy(task.request_pid, request_pid);
        strcpy(task.request, request);

        if (isEmpty()) {
            // TODO - colocar as transformações em utilização
            // write(client_server_fd, "processing\n", 12);
            // close(client_server_fd);
            int skip = skip_request_priority(request);
            int number_of_transformations = get_number_of_transformations(task.transformations);
            add_running_task(task);
            process_transformations(request + skip, request_pid, transformations_path, number_of_transformations);
        }
        else {
            // close(client_server_fd);
            enqueue(task);
        }
    }
    else {
        // write(client_server_fd, "denied\n", 8);
        // close(client_server_fd);
    }
}

void select_task(char *request, int request_id, char* transformations_path) {
    char buffer[strlen(request) + 1]; 
    strcpy(buffer, request);
    char *token = strtok(buffer, " ");
    char request_pid[32];
    strcpy(request_pid, token);
    int skip_pid = strlen(token) + 1;
    token = strtok(NULL, " ");
    if (strcmp(token, "proc-file") == 0) {
        proc_file_task(request + skip_pid, request_id, request_pid, transformations_path);
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
    running_tasks = NULL;
    char request[256];
    int requests = 0;
    while (1) {
        write(1, "ab", 2);
        int fd = open("server_client_fifo", O_RDONLY);
        int n = read_request(fd, request, 256);
        close(fd);
        requests++;
        select_task(request, requests, argv[2]);
    }
    return 0;
}