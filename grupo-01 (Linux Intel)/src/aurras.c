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

    signal(SIGALRM, handler);

    int fifo_fd = open(FIFO,O_WRONLY);
    if(fifo_fd == -1){
        puts("[Error] Server not iniciated!\n");
        return 0;
    }
    
    char* str = malloc(sizeof(char)*200);
    for (int i = 1; i < argc; i++) {
        strcat(str,argv[i]);
        strcat(str,";");
        write(fifo_fd,str,strlen(str));
    } 
    
    printf("[Escrito PID] Pronto para receber dados\n");
    close(fifo_fd);
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
    while (readFifo_fd <= 0)
        readFifo_fd = open(SENDTOCLIENT,O_RDONLY);
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
}