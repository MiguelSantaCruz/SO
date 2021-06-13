#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

#define BUFFERSIZE 1024
#define SENDTOCLIENT "sendToClient"
#define FIFO "fifo"

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

    int fifo_fd = open(FIFO,O_WRONLY);
    if(fifo_fd == -1){
        puts("[Error] Server not iniciated!\n");
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        write(fifo_fd,argv[i],strlen(argv[i]));
    }
    sleep(1);   
    write(fifo_fd,"ready",5);
    printf("[Ready] Pronto para receber dados\n");
    close(fifo_fd);
    int readFifo_fd = open(SENDTOCLIENT,O_RDONLY);
    int bytes = 0;
    char buffer[BUFFERSIZE];
    while ((bytes = read(readFifo_fd, buffer, BUFFERSIZE)) > 0) {
            if(strcmp(buffer,"none")){
                break;
            }   else{
                write(STDOUT_FILENO, buffer, bytes);
            }          
    }
    close(readFifo_fd);
    return 0;
}