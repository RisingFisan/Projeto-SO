#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define FINISHED 0
#define EXECUTING 1
#define TERMINATED 2
#define TERMINACTIVE 3
#define TERMTEXEC 4

void strcpyandtrim(char* dest, char* src);

int main(int argc, char const *argv[]) {
    mkfifo("./client_server_fifo", 0644);
    mkfifo("./server_client_fifo", 0644);
    int client_server_fifo = open("./client_server_fifo", O_RDONLY);
    int server_client_fifo = open("./server_client_fifo", O_WRONLY);
    ssize_t bytesRead = 0;

    char* processes[1024];
    int exitStatus[1024];
    int pids[1024];
    int lastProcess = 0;

    while(1) {
        char* buffer = calloc(1024, sizeof(char));
        int bytesRead = read(client_server_fifo, buffer, 1024);

        if(strncmp(buffer, "ajuda", 5) == 0) {
            char helpMessage[] = "tempo-inactividade TEMPO\n"
                                "tempo-execucao TEMPO\n"
                                "executar COMANDO1 [| COMANDO2 ...]\n"
                                "listar\n"
                                "terminar NUM_TAREFA\n"
                                "historico\n"
                                "ajuda\n"
                                "output NUM_TAREFA\n";
            write(server_client_fifo, helpMessage, strlen(helpMessage));
        }
        else if(strncmp(buffer, "executar", 8) == 0) {
            char* command = calloc(1024, sizeof(char));
            strcpyandtrim(command, buffer + 9);

            processes[lastProcess] = strdup(command);
            exitStatus[lastProcess] = EXECUTING;

            int complexCommand = 0;
            int pipes[1024][2];
            int currPipe = 0;

            char* token;
            char* args[1000];
            int i = 0;
            while((token = strtok_r(command, " ", &command))) {
                complexCommand = 1;
                if(i == 0) {
                    args[0] = strdup(token);
                    i++;
                }

                if(*token == '|') {
                    pipe(pipes[currPipe]);
                    args[i] = NULL;
                    if(fork() == 0) {
                        close(pipes[currPipe][0]);
                        if(currPipe != 0) {
                            dup2(pipes[currPipe - 1][0], STDIN_FILENO);
                            close(pipes[currPipe - 1][0]);
                        }
                        dup2(pipes[currPipe][1], STDOUT_FILENO);
                        close(pipes[currPipe][1]);
                        execvp(args[0], args + 1);
                    }
                    else {
                        i = 0;
                    }
                    close(pipes[currPipe][1]);
                    if(currPipe != 0) close(pipes[currPipe - 1][0]);
                    currPipe++;
                }
                else {
                    args[i] = strdup(token);
                    i++;
                }
            }

            args[i] = NULL;
            if(fork() == 0) {
                dup2(server_client_fifo, STDOUT_FILENO);
                close(server_client_fifo);
                if(currPipe != 0) {
                    dup2(pipes[currPipe - 1][0], STDIN_FILENO);
                    close(pipes[currPipe - 1][0]);
                }
                execvp(args[0], args + 1);
            }
            lastProcess++;
        }
        else if(strncmp(buffer, "exit", 4) == 0) {
            break;
        }
        else if(strncmp(buffer, "listar", 6) == 0) {
            char message[1024];
            for(size_t i = 0; i < lastProcess; i++) {
                if(exitStatus[i] == EXECUTING) {
                    sprintf(message, "#%zu: %s\n", i + 1, processes[i]);
                    write(server_client_fifo, message, strlen(message));
                }
            }
        }
        else if(strncmp(buffer, "historico", 9) == 0) {
            char message[1024];
            for(size_t i = 0; i < lastProcess; i++) {
                char* status;
                switch(exitStatus[i]) {
                    case FINISHED:
                        status = strdup("concluída");
                        break;
                    case TERMINATED:
                        status = strdup("terminada");
                        break;
                    case TERMINACTIVE:
                        status = strdup("max inatividade");
                        break;
                    case TERMTEXEC:
                        status = strdup("max execução");
                        break;
                    default:
                        status = strdup("em execução");
                        break;
                }
                sprintf(message, "#%zu, %s: %s\n", i + 1, status, processes[i]);
                write(server_client_fifo, message, strlen(message));
            }
        }
        else write(server_client_fifo, buffer, bytesRead);

        free(buffer);
        //close(server_client_fifo);
    }
    return 0;
}

void strcpyandtrim(char* dest, char* src) {
    char* start = src;
    while(*start == ' ') start++;
    start++;
    strcpy(dest, start);
    while(dest[strlen(dest) - 1] != '"') dest[strlen(dest) - 1] = '\0';
    dest[strlen(dest) - 1] = '\0';
}