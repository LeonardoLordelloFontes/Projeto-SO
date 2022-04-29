#include <sys/types.h>
#include <unistd.h> /* chamadas ao sistema: defs e decls essenciais */
#include <fcntl.h> /* O_RDONLY, O_WRONLY, O_CREAT, O_* */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

int main(int argc, char *argv[]) {
    /*
    int fd = open("server_client_fifo", O_WRONLY);
    if (fd == -1) {
        return 1;
    }
    char buffer2[256];
    int total_size = 0;
    for (int i = 1; i < argc; i++) {
        int size = strlen(argv[i]) + 2;
        total_size += size;
        char* buffer = malloc(size);
        snprintf(buffer, size, "%s ", argv[i]);
        strcat(buffer2, buffer);
    }
    buffer2[total_size - 2] = '\0';
    write(fd, buffer2, total_size);
    close(fd);*/

    return 0;
}