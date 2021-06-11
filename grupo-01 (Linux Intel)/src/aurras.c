#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

#define BUFFERSIZE 1024

int main(int argc, char* argv[]){
    int fifo = open("fifo",O_WRONLY);
    if(fifo == -1){
        puts("[Error] Server not iniciated!\n");
    }
    for (int i = 1; i < argc; i++) {
        write(fifo,argv[i],strlen(argv[i]));
    }
    close(fifo);
    int readFifo = open("sendToClient",O_RDONLY);
    int bytes = 0;
    char buffer[BUFFERSIZE];
    while ((bytes = read(readFifo,buffer,BUFFERSIZE)) > 0) {
            write(STDOUT_FILENO,buffer,bytes);
    }
    close(readFifo);
    return 0;
}