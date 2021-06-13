#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFFERSIZE 1024
#define SENDTOCLIENT "sendToClient"
#define FIFO "fifo"

void sendPid(int fifo_fd);
void leResposta();

void handler (int signum) {
    if (signum == SIGUSR1) {
        printf("Recebido sinal 1");
        leResposta();
        _exit(0);
    }
}

int main(int argc, char* argv[]){
    if(signal(SIGUSR1, handler) == SIG_ERR){
        perror("Sinal 1");
    }
    if (argc < 2) {
        perror ("Formato inválido");
        return -1;
    }
    int fifo_fd = open(FIFO,O_WRONLY);
    if(fifo_fd == -1){
        puts("[Error] Server not iniciated!\n");
        return 0;
    }
    char* str = malloc(sizeof(char)*200);
    for (int i = 1; i < argc; i++) {;
        write(fifo_fd,argv[i],strlen(argv[i]));
    }
    leResposta(fifo_fd);
    return 0;
}

void sendPid(int fifo_fd){
    pid_t pid = getpid();
    char str1[10];
    sprintf(str1,"%d;",pid);
    write(fifo_fd,str1,strlen(str1));
}

void leResposta(){
    int readFifo_fd = open(SENDTOCLIENT,O_RDONLY);
    int bytes = 0;
    char buffer[BUFFERSIZE];
    //write(fifo_fd,"ready",5);
    //sleep(1);
    while ((bytes = read(readFifo_fd, buffer, BUFFERSIZE)) > 0) {
            buffer[bytes] = '\0';
            if(strcmp(buffer,"none") == 0){
                printf("Recebido [NONE]\n");
            } else {
                write(STDOUT_FILENO, buffer, bytes);
            }          
    }
    close(readFifo_fd);
}