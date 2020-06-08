#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void strcpyandtrim(char* dest, char* src);

int main(int argc, char const *argv[]) {
    mkfifo("./fifo", 0644);
    int fifo = open("./fifo", O_RDONLY);
    ssize_t bytesRead = 0;

    char* inExecution[1024];
    int lastProcess = 0;

    while(1) {
        char* buffer = calloc(1024, sizeof(char));
        int bytesRead = read(fifo, buffer, 1024);
        if(strncmp(buffer, "ajuda", 5) == 0) {
            char helpMessage[] = "tempo-inactividade TEMPO\n"
                                "tempo-execucao TEMPO\n"
                                "executar COMANDO1 [| COMANDO2 ...]\n"
                                "listar\n"
                                "terminar NUM_TAREFA\n"
                                "historico\n"
                                "ajuda\n"
                                "output NUM_TAREFA\n";
            write(STDOUT_FILENO, helpMessage, strlen(helpMessage));
        }
        else if(strncmp(buffer, "executar", 8) == 0) {
            char* command = calloc(1024, sizeof(char));
            strcpyandtrim(command, buffer + 9);

            inExecution[lastProcess] = strdup(command);
            lastProcess++;

            write(STDOUT_FILENO, command, strlen(command));
            write(STDOUT_FILENO, "\n", 1);

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
                if(currPipe != 0) {
                    dup2(pipes[currPipe - 1][0], STDIN_FILENO);
                    close(pipes[currPipe - 1][0]);
                }
                execvp(args[0], args + 1);
            }
        }
        else if(strncmp(buffer, "exit", 4) == 0) {
            break;
        }
        else if(strncmp(buffer, "listar", 6) == 0) {
            char message[1024];
            for(size_t i = 0; i < lastProcess; i++) {
                sprintf(message, "#%zu: %s", i + 1, inExecution[i]);
                write(STDOUT_FILENO, message, strlen(message));
                write(STDOUT_FILENO, "\n", 1);
            }
        }
        else write(STDOUT_FILENO, buffer, bytesRead);

        free(buffer);
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