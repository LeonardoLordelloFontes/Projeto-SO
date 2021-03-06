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
char transformations_path[256];

Task remove_running_task(pid_t pid) {
    Task task;
    RunningTasks aux;
    if (pid == running_tasks->task.child_process) {
        aux = running_tasks;
        running_tasks = running_tasks->next;
    }
    else {
        RunningTasks last;
        for (aux = running_tasks; aux != NULL && aux->task.child_process != pid; aux = aux->next)
            last = aux;
        if (aux == NULL) {
            aux = last;
            last = NULL;
        }
        else {
            last->next = aux->next;
        }
    }
    task = aux->task;
    free(aux);
    return task;
}

int get_file_size(char *path) {
    struct stat st;
    stat(path, &st);
    return st.st_size;
}

void sigchld_handler(int sig) {
    int status;
    pid_t pid;
    
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        Task task = remove_running_task(pid);
        remove_user_transformations(task.transformations);
        if (WEXITSTATUS(status) == 0) {
            char in[128], out[128], buffer[128];
            get_input_output(task.request, in, out);
            int in_size = get_file_size(in);
            int out_size = get_file_size(out);
            int n = snprintf(buffer, 128, "concluded (bytes input: %d, bytes output: %d)\n", in_size, out_size);
            write(task.request_fd, buffer, n);
            close(task.request_fd);
            while (peak_transformations() != NULL && check_transformations_availableness(peak_transformations())) {
                task = dequeue();
                int number_of_transformations = get_number_of_transformations(task.transformations);
                process_transformations(task, number_of_transformations);
            }
        }
        else {
            write(task.request_fd, "something went wrong\n", 22);
            close(task.request_fd);
        }
    }
}

void sigterm_handler(int sig) {
    while(!isEmpty() || running_tasks != NULL) {
        pause();
    }
    _exit(0);
}

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

int check_transformations_availableness(int* user_transformations) {
    int available = 1;
    for (int i = 0; i < 7 && available; i++) {
        if (server_transformations[i][0] + user_transformations[i] > server_transformations[i][1])
            available = 0;
    }
    return available;
}

void add_user_transformations(int user_transformations[7]) {
    for (int  i = 0; i < 7; i++) {
        server_transformations[i][0] += user_transformations[i];
    }
}

void remove_user_transformations(int user_transformations[7]) {
    for (int  i = 0; i < 7; i++) {
        server_transformations[i][0] -= user_transformations[i];
    }
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

void status_task(char *request_pid) {
    int client_server_fd = open(request_pid, O_WRONLY);
    char buffer[512];
    char* transformations[7] = {"nop", "bcompress", "bdecompress", "gcompress", "gdecompress", "encrypt", "decrypt"};
    for (RunningTasks aux = running_tasks; aux != NULL; aux = aux->next) {
        int n = snprintf(buffer, 512, "task #%d: %s\n", aux->task.request_id, aux->task.request);
        write(client_server_fd, buffer, n);
    }
    for (int i = 0; i < 7; i++) {
        int key = get_transformation_key(transformations[i]);
        int running = server_transformations[key][0];
        int max = server_transformations[key][1];
        int n = snprintf(buffer, 512, "transf %s: %d/%d (running/max)\n", transformations[i], running, max);
        write(client_server_fd, buffer, n);
    }
    close(client_server_fd);
}

int get_number_of_transformations(int user_transformations[7]) {
    int transformations = 0;
    for (int i = 0; i < 7; i++) {
        transformations += user_transformations[i];
    }
    return transformations;
}

void get_input_output(char *request, char *input, char *output) {
    int skip = skip_request_priority(request);
    char buffer[strlen(request)+1];
    strcpy(buffer, request+skip);
    char *token = strtok(buffer, " ");
    strncpy(input, token, 128);
    token = strtok(NULL, " ");
    strncpy(output, token, 128);
}

void process_transformations(Task task, int number_of_transformations) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
    }
    if (pid == 0) {
        int pipes[number_of_transformations][2];
        pid_t transformation_pids[number_of_transformations];
        char in[128], out[128];
        get_input_output(task.request, in, out);
        char buffer[strlen(task.request)+1];
        int skip = skip_request_priority(task.request);
        strcpy(buffer, task.request+skip);
        strtok(buffer, " ");
        strtok(NULL, " ");
        
        int in_fd;
        int out_fd;

        for (int i = 0; i < number_of_transformations; i++) {
            char *token = strtok(NULL, " ");

            if (i == 0) {
                in_fd = open(in, O_RDONLY);
                if (in_fd == -1) {
                    perror("open");
                    _exit(1);
                }
            }
            else 
                in_fd = pipes[i-1][0];
            if (i == number_of_transformations - 1) {
                out_fd = open(out, O_CREAT|O_TRUNC|O_WRONLY, 0666);
                if (out_fd == -1) {
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
                char exec_transformation_path[512];
                snprintf(exec_transformation_path, 512, "./%s/%s", transformations_path, token);
                execlp(exec_transformation_path, exec_transformation_path, NULL);
                perror("exec");
                _exit(1);
            }
            else {
                transformation_pids[i] = pid_transformation;
                close(out_fd);
                close(in_fd);
            }
        }
        for (int i = 0; i < number_of_transformations; i++) {
            waitpid(transformation_pids[i], NULL, 0);
        }
        _exit(0);
    }
    else {
        write(task.request_fd, "processing\n", 12);
        add_user_transformations(task.transformations);
        task.child_process = pid;
        add_running_task(task);
    }
}

void proc_file_task(char *request, int request_id, char *request_pid) {
    Task task = {.transformations = {0}};
    fill_user_transformations(request, task.transformations);
    int client_server_fd = open(request_pid, O_WRONLY);
    if (client_server_fd == -1) {
        perror("open");
        _exit(1);
    }
    if (accept_user_request(task.transformations)) {
        write(client_server_fd, "pending\n", 9);
        task.priority = get_task_priority(request);
        task.request_id = request_id;
        task.request_fd = client_server_fd;
        strcpy(task.request, request);

        if (isEmpty() && check_transformations_availableness(task.transformations)) {
            int number_of_transformations = get_number_of_transformations(task.transformations);
            process_transformations(task, number_of_transformations);
        }
        else {
            enqueue(task);
        }
    }
    else {
        write(client_server_fd, "denied\n", 8);
        close(client_server_fd);
    }
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
        status_task(request_pid);
    }
}

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
    strcpy(transformations_path, argv[2]);
    mkfifo("server_client_fifo", 0666);
    signal(SIGCHLD, sigchld_handler);
    signal(SIGTERM, sigterm_handler);
    initQueue();
    running_tasks = NULL;
    char request[256];
    int requests = 0;
    while (1) {
        int fd = open("server_client_fifo", O_RDONLY);
        read_request(fd, request, 256);
        close(fd);
        requests++;
        select_task(request, requests);
    }
    return 0;
}