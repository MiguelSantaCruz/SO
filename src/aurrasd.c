#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

#define BUFFERSIZE 1024

void sendStatus();

int main(int argc, char* argv[]){
    char buffer[1024];
    int bytes = 0;
    mkfifo("fifo",0644);
    int fifo = open("fifo",O_RDONLY);
    while(1){
        while ((bytes = read(fifo,buffer,BUFFERSIZE)) > 0) {
            if(strcmp(buffer,"status") == 0){
                sendStatus();
            }
            write(STDOUT_FILENO,buffer,bytes);
        }
    }
    unlink("fifo");
    unlink("sendToClient");
    return 0;
}

void sendStatus(){
    mkfifo("sendToClient",0644);
    int fifo = open("sendToClient",O_WRONLY);
    char string[] = "Não implementado\n";
    write(fifo,string,strlen(string));
    close(fifo);
}