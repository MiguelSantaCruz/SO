#include "aurrasd.h"

struct data {
    int runningProcesses;
    int maxRunningProcesses;
};

void ctrl_c_handler(int signum){
    unlink("fifo");
    unlink("sendToClient");
    puts("\n[Server] Pipes fechados e servidor terminado");
    exit(0);
}

int main(int argc, char* argv[]){
    if (argc < 3) {
        perror("Formato de execução incorreto!");
        return -1;
    }
    DATA data = malloc(sizeof(struct data));
    initializeData(data);
    if(signal(SIGINT,ctrl_c_handler) == SIG_ERR){
        perror("Erro de terminação");
    }
    char buffer[1024];
    int bytes = 0;
    mkfifo("fifo",0644);
    int fifo = open("fifo",O_RDONLY);
    while(1){
        while ((bytes = read(fifo,buffer,BUFFERSIZE)) > 0) {
            if(strcmp(buffer,"status") == 0){
                sendStatus(data);
            }
            write(STDOUT_FILENO,buffer,bytes);
        }
    }
    unlink("fifo");
    unlink("sendToClient");
    return 0;
}

void sendStatus(DATA data){
    mkfifo("sendToClient",0644);            //nao teria que ser fora desta funçao (p.e. criar uma init_fifo) pq assim esta sempre a dar mkfifo... 
    int fifo = open("sendToClient",O_WRONLY);
    char string[100];
    sprintf(string,"Running Processes: [%d/%d]\n",data->runningProcesses,data->maxRunningProcesses);
    write(fifo,string,strlen(string));
    close(fifo);
}

void initializeData(DATA data){
    data->maxRunningProcesses = 3;
    data->runningProcesses = 0;
}