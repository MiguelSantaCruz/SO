#include "aurrasd.h"

#define MAX_READ_BUFFER 2048
#define MAX_BUF_SIZE 1024
#define SENDTOCLIENT "../tmp/sendToClient"
#define FIFO "fifo"
#define CONFIG_PATH "../etc/aurrasd.conf"

struct data {
    int runningProcesses;
    int maxRunningProcesses;
};

struct config {
    char id_filtro[12];
    char fich_exec[60];
    int max_inst;
};

void handler(int signum){
    switch (signum)             //optei por um switch pq assim ficam todos os sinais neste handler
    {
    case SIGINT:
        unlink(FIFO); 
        unlink(SENDTOCLIENT);
        puts("\n[Server] Pipes fechados e servidor terminado");
        exit(0);
        break;
    
    default:
        break;
    }
    
}



//------------------------------FUNÇÃO AUXILIAR (readline)-----------------------------------------------

int contador = 0 ;

int readch(int fd, char* buf){
    contador ++;
    return read(fd,buf,1);
}

int readline(int fd, char* buf, size_t size){
    char* tmp = malloc(sizeof(char*));
    int curr = 0;
    int read = 0;
    while ((read = readch(fd,tmp))>0 && curr < size)
    {
        buf[curr] = tmp [read-1];
        curr++;
        if (tmp[read-1]=='\n')
            return -1;
    }
    return 0;
}

//-------------------------------------------------------------------------------


int serverConfig (char* path, struct config * cfg) {
    char buffer[MAX_BUF_SIZE];
    
    int file_open_fd = open (path, O_RDONLY);    //começar por abrir o ficheiro para leitura
    if (file_open_fd < 0) {
        perror ("[open] Path error");
        _exit(-1);
    }


    for (int i=0; i<5; i++) {

        readline(file_open_fd, buffer, 200);
        char * substr = strtok(buffer, " ");

        strcpy (cfg->id_filtro, substr);
        substr = strtok(NULL, " ");
        strcpy (cfg->fich_exec, substr);
        substr = strtok(NULL, " ");
        cfg->max_inst = atoi(substr);

        printf("%s\n", cfg->id_filtro);
        printf ("%s\n", cfg->fich_exec);
        printf("%d\n", cfg->max_inst);
    }


    printf("Filtros carregados na struct config\n");    //para debug :)
    return 0;
}


int main(int argc, char* argv[]){
    if (argc < 3) {
        perror("Formato de execução incorreto!");
        return -1;
    }
    struct config *cfg = malloc(sizeof(struct config));
    serverConfig(CONFIG_PATH, cfg);
    DATA data = malloc(sizeof(struct data));
    initializeData(data);
    if(signal(SIGINT, handler) == SIG_ERR){
        perror("Erro de terminação");
    }
    char buffer[1024];
    int bytes = 0;
    mkfifo(FIFO, 0644);
    int fifo_fd = open(FIFO, O_RDONLY);
    while(1){
        while ((bytes = read(fifo_fd, buffer, BUFFERSIZE)) > 0) {
            if(strcmp(buffer,"status") == 0){
                sendStatus(data);
            }
            write(STDOUT_FILENO,buffer,bytes);
        }
    }
    unlink(FIFO);
    unlink(SENDTOCLIENT);
    return 0;
}

void sendStatus(DATA data){
    mkfifo("sendToClient",0644);            //nao teria que ser fora desta funçao (p.e. criar uma init_fifo) pq assim esta sempre a dar mkfifo... 
    int sendToClient_fd = open(SENDTOCLIENT, O_WRONLY);
    char string[100];
    sprintf(string, "Running Processes: [%d/%d]\n", data->runningProcesses, data->maxRunningProcesses);
    write(sendToClient_fd, string, strlen(string));
    close(sendToClient_fd);
}

void initializeData(DATA data){
    data->maxRunningProcesses = 3;
    data->runningProcesses = 0;
}