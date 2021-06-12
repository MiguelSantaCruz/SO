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
    char fich_exec[12];
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


int serverConfig (char* path) {
    char buffer[MAX_BUF_SIZE];
    
    int file_open_fd = open (path, O_RDONLY);    //começar por abrir o ficheiro para leitura
    if (file_open_fd < 0) {
        perror ("[open] Path error");
        _exit(-1);
    }

    //agora, vamos ler o que tem no ficheiro
    int nbytes = read (file_open_fd, buffer, MAX_BUF_SIZE);
    if (nbytes <= 0) {
        perror("[read] No bytes readed");
        return -1;
    }
    struct config *temp = malloc(sizeof(struct config));

    char *str;
    char *exec_args[3];
    int i = 0;
    while (readline(file_open_fd, buffer, MAX_BUF_SIZE) > 0) {
        //apos ler uma linha, vamos separá-la nos seus 3 campos e colocar na struct
        printf("banana\n");
        str = strtok(buffer, " ");
        while (str != NULL) {
            exec_args[i] = str;
            printf ("%s\n", str);   //verificar se foi bem separado quando o programa estiver a correr
            str = strtok(NULL, " ");
            i++;
        }
        printf("%s\n", exec_args[0]);
        printf("%s\n", exec_args[1]);
        printf("%s\n", exec_args[2]);

        strcpy(temp->id_filtro, exec_args[0]);
        strcpy(temp->fich_exec, exec_args[1]);
        temp->max_inst = atoi(exec_args[2]);

        //reset counter to make the same for the next lines
        i = 0;
    }
    printf("Filtros carregados na struct config\n");    //para debug :)
    return 0;
}


int main(int argc, char* argv[]){
    if (argc < 3) {
        perror("Formato de execução incorreto!");
        return -1;
    }
    serverConfig("../etc/aurrasd.conf");
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