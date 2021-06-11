#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFFERSIZE 1024

typedef struct data *DATA;

int main(int, char**);
void sendStatus(DATA);
void initializeData(DATA);
void ctrl_c_handler(int);