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

/* Perfect Hash

    nop - 4
    bcompress - 1
    bdecompress - 2
    gcompress - 6
    gdecompress - 0
    encrypt - 3
    decrypt - 5

*/

int server_transformations[7][2];

int transformation_key(char* str) {
    int key = (str[0] + str[1]) % 7;
    if (str[0] == 'e') 
        key += 2;
    return key;
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
        int key = transformation_key(token);
        token = strtok(NULL, " \n");
        server_transformations[key][0] = 0; // running 
        server_transformations[key][1] = atoi(token); // max
    }
}

ssize_t read_request(int fd, char* request, size_t size) {
    ssize_t bytes_read = read(fd, request, size);
    size_t request_size = strcspn(request, "\n") + 1;
    request[request_size] = '\0';
    lseek(fd, request_size - bytes_read, SEEK_CUR);
    return request_size;
}

int main(int argc, char *argv[]) {

    max_runnable_transformations(argv[1]);
    mkfifo("server_client_fifo", 0666);
    char buffer[256];

    while (1) {
        int fd = open("server_client_fifo", O_RDONLY);
        int n = read_request(fd, buffer, 256);
        // int aux, n = 0;
        // while ((aux = read_request(fd, buffer, 256)) > 0) n += aux;
        close(fd);
        long x;
        for (int i = 0; i < 1000000000; i++)
            x += i;
        write(1, buffer, n);
        // strncpy(user_request, buffer, n);
        //write(1, user_request, n);
        // if (n <= 255)
        // write(1, buffer, n);
        /*else
            write(1, "arroz", 5);*/
    }

    /*
    int fds[2];
    pipe(fds);
    pid_t pid3 = fork();
    if (pid3 == 0) {
        close(fds[0]);
        int fd = open("server_client_fifo", O_RDONLY);
        while (1) {
            char buffer[256];
            int n = read(fd, buffer, 256);
            write(fds[1], buffer, n);
        }
    }

    int clients = 0;
    while(1) {
        int fd = open("server_client_fifo", O_RDONLY);
        char buffer[256];
        char buffer2[256];
        int n = read(fd, buffer, 256);
        close(fd);
        clients++;
        write(1, buffer, n);
        // sprintf(buffer2, "%d", clients);
        //write(1, buffer2, strlen(buffer2));
    
    
    int fds2[2];
    pipe(fds2);

    while(1) {
        pid_t pid1 = fork();
        if (pid1 == 0) {
            close(fds[0]);
            close(fds[1]);
            close(fds2[0]);
            int y = 3;
            char buffer4[64];
            sprintf(buffer4, "%d", y);
            write(fds2[1], buffer4, strlen(buffer4));
            close(fds2[1]);
            _exit(1);
        }
        char buffer2[256];
        char buffer3[32];
    
        int n3 = read(fds2[0], buffer3, 32);
        int x = atoi(buffer3);
        printf("%d", x);
        // write(1, buffer3, n3);
        int n2 = read(fds[0], buffer2, 256);
        write(1, buffer2, n2);
        pid_t pid2 = fork();
        if (pid2 == 0) {
            kill(getpid(), SIGSTOP);
            write(1, "TODO", 4);
            _exit(0);
        }
    }

    waitpid(pid3, NULL, 0);

    while(1) {
        pid_t queue_process = fork();

        if (queue_process == -1) {
            perror("fork");
            return 1;
        }

        if (queue_process == 0) {
            sleep(2);
            write(1, "arroz", 5);
            _exit(1);
        }

        pid_t client_process = fork();

        if (client_process == -1) {
            perror("fork");
            return 1;
        }

        if (client_process == 0) {
            clients++;
            int fd = open("server_client_fifo", O_RDONLY);
            char buffer[256];
            int n = read(fd, buffer, 256);
            close(fd);
            write(fds[1], buffer, n);
            _exit(0);
        }

        if (fork() == 0) {

            _exit(0);
        }

        waitpid(queue_process, NULL, 0);
    }*/
    return 0;
}