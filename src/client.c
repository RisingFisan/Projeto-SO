#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

ssize_t readline(int fd, char* buf, int size);

void sigchld_handler(int sig) {
    wait(NULL);
}

int main(int argc, char const *argv[]) {
    int client_server_fifo = open("client_server_fifo", O_WRONLY);
    char string[1024];
    
    signal(SIGCHLD, sigchld_handler);

    char stuck_on_write[] = "stuck on write\n";
    char stuck_on_read[] = "stuck on read\n";

    if(argc < 2) {
        char string[1024];
        int bytesRead = 0;
        while((bytesRead = readline(STDIN_FILENO, string, 1024)) > 0) {
            write(client_server_fifo, string, bytesRead);
        
            if(fork() == 0) {
                int server_client_fifo = open("server_client_fifo", O_RDONLY);
                while((bytesRead = read(server_client_fifo, string, 1024)) > 0) {
                    write(STDOUT_FILENO, string, bytesRead);
                }
                close(server_client_fifo);
                exit(0);
            }
        }
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
            write(client_server_fifo, string, strlen(string));
        }
        int bytesRead = 0;
        int server_client_fifo = open("server_client_fifo", O_RDONLY);
        while((bytesRead = read(server_client_fifo, string, 1024)) > 0) {
            write(STDOUT_FILENO, string, bytesRead);
        }
        close(server_client_fifo);
    }
    return 0;
}

ssize_t readline(int fd, char* buf, int size) {
    ssize_t bytesRead = read(fd, buf, size);
    if(!bytesRead) return 0;

    if(buf[bytesRead - 1] == '\n') buf[--bytesRead] = 0;
    return bytesRead;
}