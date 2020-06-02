#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char const *argv[]) {
    int fifo = open("/tmp/fifo", O_WRONLY);
    char string[1024];
    
    if(argc < 2) {
        char string[1024];
        int bytesRead = 0;
        while((bytesRead = read(STDIN_FILENO, string, 1024)) > 0) 
            write(fifo, string, bytesRead);        
    }
    else {
        if(*argv[1] == '-') {
            switch(argv[1][1]) {
                case 'h':
                    strcpy(string, "ajuda\n");
                    break;
                case 'r':
                    strcpy(string, "historico\n");
                    break;
                case 't':
                    strcpy(string, "terminar\n");
                    break;
                case 'l':
                    strcpy(string, "listar\n");
                    break;
                case 'e':
                    sprintf(string, "executar %s\n", argv[2]);
                    break;
                case 'm':
                    sprintf(string, "tempo-execucao %s\n", argv[2]);
                    break;
                case 'i':
                    sprintf(string, "tempo-inactividade %s\n", argv[2]);
                    break;
                case 'o':
                    sprintf(string, "output %s\n", argv[2]);
                    break;
            }
            write(fifo, string, strlen(string));
        }
    }
    return 0;
}
