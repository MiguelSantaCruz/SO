#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

#define BUFFERSIZE 1024


void handler (int signum) {
    if (signum == SIGALRM) {
        _exit(0);
    }
}



int main(int argc, char* argv[]){
    if (argc < 2) {
        perror ("Formato inválido");
        return -1;
    }

    signal(SIGALRM, handler);

    int fifo_fd = open("fifo",O_WRONLY);
    if(fifo_fd == -1){
        puts("[Error] Server not iniciated!\n");
    }

    for (int i = 1; i < argc; i++) {
        write(fifo_fd,argv[i],strlen(argv[i]));
    }

    close(fifo_fd);
    int readFifo_fd = open("sendToClient",O_RDONLY);
    int bytes = 0;
    char buffer[BUFFERSIZE];
    while ((bytes = read(readFifo_fd, buffer, BUFFERSIZE)) > 0) {
            write(STDOUT_FILENO, buffer, bytes);
    }
    close(readFifo_fd);
    return 0;
}