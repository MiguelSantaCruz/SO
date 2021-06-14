#include "aurrasd.h"
#include "queue.h"

#define MAX_READ_BUFFER 2048
#define MAX_BUF_SIZE 1024
#define SENDTOCLIENT "sendToClient"
#define FIFO "fifo"
#define CONFIG_PATH "../etc/aurrasd.conf"
#define NUMBER_OF_FILTERS 5

struct config {
    int* runningProcesses;            //processos a correr de momento
    char** execFiltros;               //aurrasd-gain-double
    int* maxInstancias;               // 2
    char** identificadorFiltro;       //alto
};


struct Queue* alto_q;
struct Queue* baixo_q;
struct Queue* eco_q;
struct Queue* rapido_q;
struct Queue* lento_q;

int em_exec_alto = 0;
int em_exec_baixo = 0;
int em_exec_eco = 0;
int em_exec_rapido = 0;
int em_exec_lento = 0;



void handler(int signum){
    switch (signum)             //optei por um switch pq assim ficam todos os sinais neste handler
    {
    case SIGINT:
        unlink(FIFO); 
        unlink(SENDTOCLIENT);
        puts("\n[Server] Pipes fechados e servidor terminado");
        exit(0);
        break;
    case SIGPIPE:
        printf("[SIGPIPE] O cliente fechou o terminal de escrita\n");
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


int serverConfig (char* path, CONFIG cfg) {     //faz o povoamento da struct com os maxInstancias
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
        cfg -> execFiltros[i] = malloc(sizeof(char)*100);
        strcpy ((char*) cfg->execFiltros[i], substr);
        substr = strtok(NULL, " ");
        cfg->maxInstancias[i] = atoi(substr);
    }
    for (int i = 0; i < NUMBER_OF_FILTERS; i++)
        printf("%s: %d\n",cfg->execFiltros[i],cfg->maxInstancias[i]);
    return 0;
}



int filtro_existente (char* nomeFiltro, CONFIG cfg) {
    for (int i = 0; i<NUMBER_OF_FILTERS; i++) {
        if (strcmp (cfg->execFiltros[i], nomeFiltro) == 0) {
            return i;           //caso encontre, retornamos o indice em que encontramos
        }
    } 
    return -1;  //caso dê erro retornamos -1
}


int filtro_permitido (int idx_filtro, CONFIG cfg) {
    //return 1 se for possivel; return 0 se nao or possivel
    if (cfg->maxInstancias[idx_filtro] > cfg->runningProcesses[idx_filtro]) {
        return 1;
    }
    return 0;
}

        //nome do fitro precisa de ser, por ex: "aurrasd-gain-double"
void execTarefa (char* nomeFiltro, CONFIG cfg, char* argv[]) {
    int indx_filtro;
    if ((indx_filtro = filtro_existente(nomeFiltro, cfg)) != -1 && filtro_permitido(indx_filtro, cfg) == 1) {
        int status;
        int pid = fork();
        if (pid == 0) {
            //coisas do filho
            char * path = strcat ("../bin/aurrasd_filters/", nomeFiltro);   //se isto nao funfar, testar com: execl(aurrasd-filters/aurrasd-echo, aurrasd-echo, NULL)
            execl(path, argv[2], NULL);     //por ex: bin/aurrasd-filters/aurrasd-echo < samples/samples-1.m4a  (visto de acordo com o README)
            printf("Exec error (não é suposto aparecer isto dps de um exec)");  //para debugg :)
        }
        else {
            //coisas do pai
            wait(&status);
            
            if (WIFEXITED(status)) {
                if (WEXITSTATUS(status) < 255) {
                    printf("[pai] tudo ok");
                }
                else {
                    printf("[pai] erros...");
                }
            }
        }
    }
    else {
        printf ("Não pode submeter mais pedidos. A lista de espera está cheia\n");
    }
}

int encontraIndice (char* identificador, struct config *cfg) {
    for (int i=0; i<5; i++) {
        if (strcmp (cfg->identificadorFiltro[i], identificador) == 0) {
            return i;
        }
    }
}

void decide_exec (CONFIG cfg, struct Queue* alto_q, struct Queue* baixo_q, struct Queue* eco_q, struct Queue* rapido_q, struct Queue* lento_q, char* argv[]) {
    int idx_alto = encontraIndice ("alto", cfg);
    if (em_exec_alto < cfg->maxInstancias[idx_alto]) {
        execTarefa(cfg->execFiltros[idx_alto], cfg, argv);
        cfg->runningProcesses[idx_alto]--;
    }
    else {
        enQueue(alto_q, argv[3]);      //guardar o nome do ficheiro de output na queue para poder fazer mais tarde
    }

    int idx_baixo = encontraIndice("baixo", cfg);
    if (em_exec_baixo < cfg->maxInstancias[idx_baixo]) {
        execTarefa(cfg->execFiltros[idx_baixo], cfg, argv);
        cfg->runningProcesses[idx_baixo]--;
    }
    else {
        enQueue(baixo_q, argv[3]);
    }

    int idx_eco = encontraIndice("eco", cfg);
    if (em_exec_eco < cfg->maxInstancias[idx_eco]) {
        execTarefa(cfg->execFiltros[idx_eco], cfg, argv);
        cfg->runningProcesses[idx_eco]--;
    }
    else {
        enQueue(eco_q, argv[3]);
    }

    int idx_rapido = encontraIndice("rapido", cfg);
    if (em_exec_rapido < cfg->maxInstancias[idx_rapido]) {
        execTarefa(cfg->execFiltros[idx_rapido], cfg, argv);
        cfg->runningProcesses[idx_rapido]--;
    }
    else {
        enQueue(rapido_q, argv[3]);
    }

    int idx_lento = encontraIndice("lento", cfg);
    if (em_exec_lento < cfg->maxInstancias[idx_lento]) {
        execTarefa(cfg->execFiltros[idx_lento], cfg, argv);
        cfg->runningProcesses[idx_lento]--;
    }
    else {
        enQueue(lento_q, argv[3]);
    }
}









/*

char *add_path(char *arg, char *path , char *f_name){ //adiciona o path ao argumento
    free(arg);
    arg=malloc(200);
    sprintf(arg, "%s/%s", path , f_name);
    return arg;
}


            //  argv do cliente | n_commands é o argc do cliente | 
int execTarefa2 (char * args[], int n_commands, char* pathFiltro, struct config * cfg) {
    char* nome_inputFile = strdup(args[2]);
    int input_fd = open (nome_inputFile, O_RDONLY, 0666);
    char* nome_outputFile = strdup(args[3]);    //pegar no filtro a applicar
    int output_fd = open (nome_outputFile, O_CREAT | O_TRUNC | O_WRONLY, 0666); //pegar no ficheiro que queremos ter

    int i, j, k;
    int p[2];
    //int n_filtros = n_commands-4;
    char ** filtros_aplicar = malloc (sizeof(char*) * (n_commands-4)); //por ex: se aplicarmos os filtros alto eco rapido
    for (i=0; i<n_commands-4; i++) {
        filtros_aplicar[i] = args[i+4];     //povoar o array com os filtros a executar
    }

    int iguais = 0;    //flag que levamta ao encontrar a posiçao do filtro (?)
    for (j=0; j<n_commands-4; j++) {
        for(k=0; k<5 && !iguais; k++) {
            if (strcmp(cfg->execFiltros[k], filtros_aplicar[j]) == 0) {
                iguais = 1;
            }
        }
        //neste ponto sabemos que k=posiçao do filtro no cfg e j=indice do filtro que aplicamos no argv
        if (iguais && (cfg->runningProcesses[k]<cfg->maxInstancias[k])) {   //alterar a alcunha do filtro (eco) por o nome do file (aurrasd-echo)
            args[i+4] = add_path(args[i+4], pathFiltro, cfg->execFiltros[k]);
            cfg->runningProcesses[k]++;
        }
        else {
            //nao fazer nada (?)
            //dica: fazer um clone do filtro no inicio e devolver esse aqui (pq estavamos a alterar o outro até este ponto)
            return -1;
        }
    }

    //redirecionamento (nome_inputfile, nome_outputfile)
    dup2(input_fd, 0);
    close (input_fd);
    dup2(output_fd, 1);
    close(output_fd);

    for (i=0; i<n_commands; i++)

}
*/


int main(int argc, char* argv[]){
    //inicializar as queues:
    alto_q = createQueue();
    baixo_q = createQueue();
    eco_q = createQueue();
    rapido_q = createQueue();
    lento_q = createQueue();


    if(signal(SIGINT, handler) == SIG_ERR){
        perror("Erro de terminação");
    }
    signal(SIGPIPE,handler);
    if (argc < 3) {
        perror("Formato de execução incorreto!");
        return -1;
    }
    CONFIG cfg = malloc(sizeof(struct config));
    initializeConfig(cfg);
    serverConfig(CONFIG_PATH, cfg);
    char buffer[1024];
    int bytes = 0;
    mkfifo(FIFO,0777);
    mkfifo(SENDTOCLIENT,0777);
    int fifo_fd = open(FIFO, O_RDONLY);
    while(1){
        while ((bytes = read(fifo_fd, buffer, BUFFERSIZE)) > 0) {
            buffer[bytes]='\0';
            printf("Lido: %s\n",buffer);
            fflush(stdout);
            if(strcmp(buffer,"status") == 0){
                printf("Recebido [STATUS]\n");
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
    int sendToClient_fd = open(SENDTOCLIENT, O_WRONLY);
    write(sendToClient_fd, "none", 4);
    close(sendToClient_fd);
    printf("Enviado [NONE]\n");
    fflush(stdout);
}

void sendStatus(CONFIG cfg){
    int sendToClient_fd = open(SENDTOCLIENT, O_WRONLY);
    char string[200];
    for (int i = 0; i < NUMBER_OF_FILTERS; i++)
    {
        sprintf(string, "Running Processes of %s: [%d/%d]\n",cfg -> execFiltros[i] ,cfg->runningProcesses[i], cfg->maxInstancias[i]);
        write(sendToClient_fd, string, strlen(string));
        printf("Escrito %s para o cliente\n",string);
        fflush(stdout);
    }
    printf("Enviado status\n");
    close(sendToClient_fd);
}

void initializeConfig(CONFIG cfg){
    cfg -> execFiltros = malloc(sizeof(char*)*NUMBER_OF_FILTERS);
    cfg -> maxInstancias = malloc(sizeof(int)*NUMBER_OF_FILTERS);
    cfg -> runningProcesses = malloc(sizeof(int)*NUMBER_OF_FILTERS);
}