#include "aurrasd.h"
#include "queue.h"

#define MAX_READ_BUFFER 2048
#define MAX_BUF_SIZE 1024
#define SENDTOCLIENT "sendToClient"
#define FIFO "fifo"
#define CONFIG_PATH "../etc/aurrasd.conf"
#define NUMBER_OF_FILTERS 5
#define STRING_SIZE 200

struct config {
    char* filtersPath;                //Pasta onde estão os filtros (bin/aurras-filters)
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
        perror ("[Erro] Ficheiro de configuração inválido :");
        _exit(-1);
    }
    for (int i=0; i<NUMBER_OF_FILTERS; i++) {
        readline(file_open_fd, buffer, BUFFERSIZE);
        char * substr = strtok(buffer, " ");
        cfg -> identificadorFiltro[i] = malloc(sizeof(char)*100);
        strcpy((char*) cfg -> identificadorFiltro[i],substr);
        substr = strtok(NULL, " ");
        cfg -> execFiltros[i] = malloc(sizeof(char)*100);
        strcpy ((char*) cfg->execFiltros[i], substr);
        substr = strtok(NULL, " ");
        cfg->maxInstancias[i] = atoi(substr);
    }
    for (int i = 0; i < NUMBER_OF_FILTERS; i++)
        printf("%s %s: %d\n",cfg->identificadorFiltro[i],cfg->execFiltros[i], cfg->maxInstancias[i]);
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
void execTarefa_fail (char* nomeFiltro, CONFIG cfg, char* argv[]) {
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
    return -1;
}



void set_read(int* lpipe){
    dup2(lpipe[0], STDIN_FILENO);
    close(lpipe[0]); 
    close(lpipe[1]); 
}
  
void set_write(int* rpipe){
    dup2(rpipe[1], STDOUT_FILENO);
    close(rpipe[0]); 
    close(rpipe[1]); 
}

void set_inputFile(int input,int* rpipe){
    dup2(input, STDIN_FILENO);
    dup2(rpipe[1],STDOUT_FILENO);
    close(rpipe[0]);
}

void set_outputFile(int output,int* lpipe){
    dup2(output, STDOUT_FILENO);
    dup2(lpipe[0],STDIN_FILENO);
    close(lpipe[1]); 
}

void fork_and_chain(int* lpipe, int* rpipe,char* filterPath,int input_fd,int output_fd){
    if(!fork())
    {
        if(input_fd != -1){
            set_inputFile(input_fd,rpipe);
        } else if (output_fd != -1){
            set_outputFile(output_fd,lpipe);
        }else {
            if(lpipe)
                set_read(lpipe);
            if(rpipe)
                set_write(rpipe);
        }
        execl(filterPath,filterPath,NULL);
    }
}

void executaTarefa (int n_filtros,char ** filtros_args, char * input_file, char * output_name, CONFIG cfg) {
    int p1[2];
    int p2[2];
    char * path = malloc(sizeof(char)*100);
    //strcat(path,"./"); vai ser util dps para por em /tmp ou ot local
    strcat(path,output_name);
    int inputFile_fd = open(input_file, O_RDONLY, 0666);
    if(inputFile_fd == -1){
        perror("ficheiro inexistente");
        sendTerminate();
        return;
    }
    for (int i = 0; filtros_args[i] != NULL ; i++)
    {
        printf("%d: %s\n",i,filtros_args[i]);   
    }
    int outputFile_fd = open(path, O_CREAT | O_RDWR, 0666);
    int pipeAtual = 0;
    pipe(p1);
    pipe(p2);
    if(fork() == 0){
        for (int i = 0; i < n_filtros; i++) {
            if (fork() == 0) {
                if(pipeAtual == 0) {
                    close(p1[0]);
                    dup2 (inputFile_fd,STDIN_FILENO);
                    close(p1[1]);
                    if(pipeAtual == n_filtros-1) {
                        dup2(outputFile_fd,STDOUT_FILENO);
                        close(p2[0]);
                        close(p2[1]);
                    }else dup2 (p2[0],STDOUT_FILENO);
                    execl(filtros_args[3+pipeAtual],filtros_args[3+pipeAtual],NULL);
                } else if(pipeAtual == n_filtros-1) {
                    if(pipeAtual % 2 != 0){
                        dup2(p2[0],STDIN_FILENO);
                        dup2(outputFile_fd,STDOUT_FILENO);
                        close(p1[0]);
                        close(p1[1]);
                        close(p2[1]);
                    } else{
                        dup2(p1[0],STDIN_FILENO);
                        dup2(outputFile_fd,STDOUT_FILENO);
                        close(p2[0]);
                        close(p2[1]);
                        close(p1[1]);
                    }
                    execl(filtros_args[3+pipeAtual],filtros_args[3+pipeAtual],NULL);
                } else {
                    if(i%2 != 0){
                        dup2(p2[0],STDIN_FILENO);
                        dup2(p1[0],STDOUT_FILENO);
                        close(p1[1]);
                        close(p2[1]);
                    }else {
                        dup2(p1[0],STDIN_FILENO);
                        dup2(p2[0],STDOUT_FILENO);
                        close(p1[1]);
                        close(p2[1]);
                    }
                    execl(filtros_args[3+pipeAtual],filtros_args[3+pipeAtual],NULL);
                }
                pipeAtual++;
                _exit(0);
            } else { 
                wait(NULL);
                //pipeAtual++;
                cfg -> runningProcesses[0]++;
                close(inputFile_fd);
                close(outputFile_fd);
                printf("Esperando filho terminar\n");
                
                printf("Terminou\n");
                printf("Terminando tarefa -1\n");
                cfg->runningProcesses[0]--;
                _exit(0);   
            }
        }
    } else {
    }
    /*
    if(fork() == 0){
        if(fork() == 0) {
            printf("Começando nova tarefa +1\n");
            dup2(inputFile_fd,STDIN_FILENO);
            dup2(outputFile_fd,STDOUT_FILENO);
            execl(filtros_args[3],filtros_args[3],NULL);
            _exit(0);
        }else { 
            cfg -> runningProcesses[0]++;
            close(inputFile_fd);
            close(outputFile_fd);
            printf("Esperando filho terminar\n");
            wait(NULL);
            printf("Terminou\n");
            printf("Terminando tarefa -1\n");
            cfg->runningProcesses[0]--;
            _exit(0);   
        }
    } else {
    }*/
    
}


int main(int argc, char* argv[]){
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
    serverConfig(argv[1], cfg);
    cfg ->filtersPath = argv[2];
    printf("Pasta dos filtros: %s\n",cfg->filtersPath);
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
            char** args = splitWord(buffer,cfg);
            if(strcmp(args[0],"status") == 0){
                printf("Recebido [STATUS]\n");
                sendStatus(cfg);
            } else if(strcmp(args[0],"transform") == 0){
                printf("Recebido [Transform]\n");
                printf("Numero de filtros: %d\n",numeroFiltros(args));
                sendTerminate();
                executaTarefa(numeroFiltros(args),args,args[1],args[2],cfg);
            } else{
                sendTerminate();
            }
        }
    }
    unlink(FIFO);
    unlink(SENDTOCLIENT);
    return 0;
}

char** splitWord(char* str,CONFIG cfg){
    char** args = malloc(sizeof(char)*10*100);
    args[0] = malloc(sizeof(char)*STRING_SIZE);
    char* token;
    token = strtok(str," ");
    strcpy(args[0],token);
    int i = 1;
    int index = 0;
    while ((token = strtok(NULL," ")) != NULL){
        args[i] = malloc(sizeof(char)*STRING_SIZE);
        if (i > 2){
            index = encontraIndice(token,cfg);
            fflush(stdout);
            if(index >= 0 && index < NUMBER_OF_FILTERS) {
                strcat(args[i],cfg->filtersPath);
                strcat(args[i],"/");
                strcat(args[i],(char*) (cfg->execFiltros[index]));
            }
        }else
            strcpy(args[i],token);
        i++;
    }
    for (int i = 0; args[i] != NULL ; i++)
    {
        printf("%d: %s\n",i,args[i]);   
    }
    return args;
}


int numeroFiltros(char** args){
    int i = 0;
    for (i = 0; args[i]!=NULL; i++);
    return i - 3;
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
        sprintf(string, "filter %s: %d/%d (running/max)\n",cfg -> identificadorFiltro[i] ,cfg->runningProcesses[i], cfg->maxInstancias[i]);
        write(sendToClient_fd, string, strlen(string));
        printf("Escrito %s para o cliente\n",string);
        fflush(stdout);
    }
    pid_t pid = getpid();
    sprintf(string,"pid: %d\n",pid);
    write(sendToClient_fd,string,strlen(string));
    printf("Enviado status\n");
    close(sendToClient_fd);
}

void initializeConfig(CONFIG cfg){
    cfg -> identificadorFiltro = malloc(sizeof(char*)*NUMBER_OF_FILTERS);
    cfg -> execFiltros = malloc(sizeof(char*)*NUMBER_OF_FILTERS);
    cfg -> maxInstancias = malloc(sizeof(int)*NUMBER_OF_FILTERS);
    cfg -> runningProcesses = malloc(sizeof(int)*NUMBER_OF_FILTERS);
}