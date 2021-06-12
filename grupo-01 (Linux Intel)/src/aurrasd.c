#include "aurrasd.h"

#define MAX_READ_BUFFER 2048
#define MAX_BUF_SIZE 1024
#define SENDTOCLIENT "../tmp/sendToClient"
#define FIFO "../tmp/fifo"

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



char read_buffer[MAX_READ_BUFFER];
int read_buffer_pos = 0;
int read_buffer_end = 0;

int readc (int fd, char *c) {
    if(read_buffer_pos == read_buffer_end){
		read_buffer_end = read(fd, read_buffer, MAX_READ_BUFFER);
		switch(read_buffer_end){        //read_buffer_end guarda o nº de bytes lidos / 0 se o ficherio terminar / -1 se ocorrer erros
			case -1:
				perror("read");
				return -1;
			case 0:
				return 0;
				break;
			default:
				read_buffer_pos = 0;
		}
	}
    *c = read_buffer[read_buffer_pos++];

    return 1;
}



ssize_t readln(int fd, char *line, size_t size) {       //line é um buffer
    int res = 0;
    int i = 0;

    while (i<size && (res = readc (fd, &line[i]) > 0)) { //ate o readc dize que ja nao ha mais nada para ler ou excedermos o tamanho max
        i++;
        if (((char*) line) [i-1] == '\n') {
            return i;   //se encontrar um \n dá imediatamente return do nº de bytes lidos
        }
    }
    return i;
}

//-------------------------------------------------------------------------------


void serverConfig (char path[200]) {
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
    while (readln(file_open_fd, buffer, MAX_BUF_SIZE) > 0) {
        //apos ler uma linha, vamos separá-la nos seus 3 campos e colocar na struct
        str = strtok(buffer, " ");
        while (str != NULL) {
            exec_args[i] = str;
            printf ("%s\n", str);   //verificar se foi bem separado quando o programa estiver a correr
            str = strtok(NULL, " ");
            i++;
        }
        strcpy(temp->id_filtro, exec_args[0]);
        strcpy(temp->fich_exec, exec_args[1]);
        strcpy(temp->max_inst, atoi(exec_args[2]));

        //reset counter to make the same for the next lines
        i = 0;
    }
    printf("Filtros carregados na struct config\n");    //para debug :)
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