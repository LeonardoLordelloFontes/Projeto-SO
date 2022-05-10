#include <sys/types.h>
#include <unistd.h> /* chamadas ao sistema: defs e decls essenciais */
#include <fcntl.h> /* O_RDONLY, O_WRONLY, O_CREAT, O_* */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

int main(int argc, char *argv[]) {
    char pid[32];
    snprintf(pid, 32, "%d", getpid());
    mkfifo(pid, 0666);
    
    int fd = open("server_client_fifo", O_WRONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    if (argc < 2) {
        fprintf(stderr, "usage: ./client option");
        return 1;
    }
   
    char buffer[256];
    strcpy(buffer, pid);
    strcat(buffer, " ");

    for (int i = 1; i < argc - 1; i++) {
        strcat(buffer, argv[i]);
        strcat(buffer, " ");
    }

    strcat(buffer, argv[argc - 1]);
    strcat(buffer, "\n");
    write(fd, buffer, strlen(buffer) + 1);
    close(fd);
    fd = open(pid, O_RDONLY);
    int n;
    while ((n = read(fd, buffer, 256)) > 0)
        write(1, buffer, n);
    close(fd);
    unlink(pid);
    return 0;
}