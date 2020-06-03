#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

ssize_t readline(int fd, char* buf, int size);

int main(int argc, char const *argv[]) {
    int fifo = open("./fifo", O_WRONLY);
    char string[1024];
    
    if(argc < 2) {
        char string[1024];
        int bytesRead = 0;
        while((bytesRead = readline(STDIN_FILENO, string, 1024)) > 0) 
            write(fifo, string, bytesRead);        
    }
    else {
        if(*argv[1] == '-') {
            switch(argv[1][1]) {
                case 'h':
                    strcpy(string, "ajuda");
                    break;
                case 'r':
                    strcpy(string, "historico");
                    break;
                case 't':
                    sprintf(string, "terminar %s", argv[2]);
                    break;
                case 'l':
                    strcpy(string, "listar");
                    break;
                case 'e':
                    sprintf(string, "executar \"%s\"", argv[2]);
                    break;
                case 'm':
                    sprintf(string, "tempo-execucao %s", argv[2]);
                    break;
                case 'i':
                    sprintf(string, "tempo-inactividade %s", argv[2]);
                    break;
                case 'o':
                    sprintf(string, "output %s", argv[2]);
                    break;
            }
            write(fifo, string, strlen(string));
        }
    }
    return 0;
}

ssize_t readline(int fd, char* buf, int size) {
    ssize_t bytesRead = read(fd, buf, size);
    if(!bytesRead) return 0;

    if(buf[bytesRead - 1] == '\n') buf[--bytesRead] = 0;
    return bytesRead;
}