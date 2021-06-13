#include "aurrasd.h"

#define MAX_READ_BUFFER 2048
#define MAX_BUF_SIZE 1024
#define SENDTOCLIENT "sendToClient"
#define FIFO "fifo"
#define CONFIG_PATH "../etc/aurrasd.conf"
#define NUMBER_OF_FILTERS 5

struct config {
    int* runningProcesses;
    char** nomes;
    int* valores;
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


//------------------------------FUNÇÃO AUXILIAR (readline) para o serverConfig -----------------------------------------------

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


int serverConfig (char* path, CONFIG cfg) {     //faz o povoamento da struct com os valores
    char buffer[MAX_BUF_SIZE];
    int file_open_fd = open (path, O_RDONLY);    //começar por abrir o ficheiro para leitura
    if (file_open_fd < 0) {
        perror ("[open] Path error");
        _exit(-1);
    }
    for (int i=0; i<NUMBER_OF_FILTERS; i++) {
        readline(file_open_fd, buffer, BUFFERSIZE);
        char * substr = strtok(buffer, " ");
        substr = strtok(NULL, " ");
        cfg -> nomes[i] = malloc(sizeof(char)*100);
        strcpy ((char*) cfg->nomes[i], substr);
        substr = strtok(NULL, " ");
        cfg->valores[i] = atoi(substr);
    }
    for (int i = 0; i < NUMBER_OF_FILTERS; i++)
        printf("%s: %d\n",cfg->nomes[i],cfg->valores[i]);
    return 0;
}



int filtro_existente (char* nomeFiltro, CONFIG cfg) {
    for (int i = 0; i<NUMBER_OF_FILTERS; i++) {
        if (strcmp (cfg->nomes[i], nomeFiltro)) {
            return i;           //caso encontre, retornamos o indice em que encontramos
        }
    } 
    return -1;  //caso dê erro retornamos -1
}


int filtro_permitido (int idx_filtro, CONFIG cfg) {
    //return 1 se for possivel; return 0 se nao or possivel
    if (cfg->valores[idx_filtro] > cfg->runningProcesses[idx_filtro]) {
        return 1;
    }
    return 0;
}


void execTarefa (char* nomeFiltro, char* args, CONFIG cfg) {
    int indx_filtro;
    if ((indx_filtro = filtro_existente(nomeFiltro, cfg)) != -1 && filtro_permitido(indx_filtro, cfg) == 1) {
        int status;
        int pid = fork();
        if (pid == 0) {
            //coisas do filho
            char * path = strcat ("../bin/aurrasd_filters/", nomeFiltro);   //se isto nao funfar, testar com: execl(aurrasd-filters/aurrasd-echo, aurrasd-echo, NULL)
            execl(path, nomeFiltro, NULL);
            printf("Exec error (não é suposto aparecer isto dps de um exec)");  //para debugg :)
        }
        else {
            //coisas do pai
            wait(&status);
        }
    }
    else {
        printf ("Não pode submeter mais pedidos. A lista de espera está cheia\n");
    }
}





int main(int argc, char* argv[]){
    if(signal(SIGINT, handler) == SIG_ERR){
        perror("Erro de terminação");
    }
    if (argc < 3) {
        perror("Formato de execução incorreto!");
        return -1;
    }
    CONFIG cfg = malloc(sizeof(struct config));
    initializeConfig(cfg);
    serverConfig(CONFIG_PATH, cfg);
    fflush(stdout);
    char buffer[1024];
    int bytes = 0;
    mkfifo(FIFO, 0744);
    mkfifo(SENDTOCLIENT,0744);
    int fifo_fd = open(FIFO, O_RDONLY);
    printf("Abertos fifos a espera de input\n");
    fflush(stdout);
    while(1){
        while ((bytes = read(fifo_fd, buffer, BUFFERSIZE)) > 0) {
            buffer[bytes]='\0';
            printf("Lido: %s\n",buffer);
            fflush(stdout);
            if(strcmp(buffer,"status") == 0){
                printf("Recebido [STATUS]\n");
                read(fifo_fd, buffer, BUFFERSIZE);
                buffer[bytes-1]='\0';
                while (strcmp(buffer,"ready") != 0)
                {
                    //printf("Ciclo ate ready\n");
                    read(fifo_fd, buffer, BUFFERSIZE);
                }
                printf("Recebido [READY]\n");
                sendStatus(cfg);
            }else{
                sendTerminate();
            }
        }
    }
    unlink(FIFO);
    unlink(SENDTOCLIENT);
    return 0;
}

void sendTerminate(){
    mkfifo(SENDTOCLIENT,0644);
    int sendToClient_fd = open(SENDTOCLIENT, O_WRONLY);
    write(sendToClient_fd, "none", 4);
    printf("Enviado [NONE]");
}

void sendStatus(CONFIG cfg){
    mkfifo(SENDTOCLIENT,0644);
    int sendToClient_fd = open(SENDTOCLIENT, O_WRONLY);
    char string[100];
    for (int i = 0; i < NUMBER_OF_FILTERS; i++)
    {
        sprintf(string, "Running Processes of %s: [%d/%d]\n",cfg -> nomes[i] ,cfg->runningProcesses[i], cfg->valores[i]);
        write(sendToClient_fd, string, strlen(string));
    }
    close(sendToClient_fd);
}

void initializeConfig(CONFIG cfg){
    cfg -> nomes = malloc(sizeof(char*)*NUMBER_OF_FILTERS);
    cfg -> valores = malloc(sizeof(int)*NUMBER_OF_FILTERS);
    cfg -> runningProcesses = malloc(sizeof(int)*NUMBER_OF_FILTERS);
}