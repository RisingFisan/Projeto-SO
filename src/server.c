#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#define FINISHED 0
#define EXECUTING 1
#define TERMINATED 2
#define TERMINACTIVE 3
#define TERMTEXEC 4

char* processes[1024];
int exitStatus[1024];
int pids[1024];
int lastProcess = 0;

void strcpyandtrim(char* dest, char* src);

void sigchld_handler(int sig) {
    pid_t pid = wait(NULL);
    for(size_t i = 0; i < lastProcess; i++) {
        if(exitStatus[i] == EXECUTING && pids[i] == pid) {
            exitStatus[i] = FINISHED;
            break;
        }
    }
}

int main(int argc, char const *argv[]) {
    mkfifo("/tmp/client_server_fifo", 0644);
    mkfifo("/tmp/server_client_fifo", 0644);

    signal(SIGCHLD, sigchld_handler);

    int client_server_fifo = open("/tmp/client_server_fifo", O_RDONLY);
    int server_client_fifo = open("/tmp/server_client_fifo", O_WRONLY);
    int log = open("log", O_RDWR | O_CREAT | O_TRUNC, 0644);
    int logidx = open("log.idx", O_RDWR | O_CREAT | O_TRUNC, 0644);
    ssize_t bytesRead = 0;

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

            char message[64];
            sprintf(message, "nova tarefa #%d\n", lastProcess);
            write(server_client_fifo, message, strlen(message));

            pid_t pid;
            if((pid = fork()) == 0) {
                int pipes[1024][2];
                int currPipe = 0;

                char* token;
                char* args[1000];
                int i = 0;
                while((token = strtok_r(command, " ", &command))) {
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
                    dup2(log, STDOUT_FILENO);
                    close(log);
                    if(currPipe != 0) {
                        dup2(pipes[currPipe - 1][0], STDIN_FILENO);
                        close(pipes[currPipe - 1][0]);
                    }
                    execvp(args[0], args + 1);
                }

                for(size_t j = 0; j <= currPipe; j++) {
                    wait(NULL);
                }
                
                off_t end = lseek(log, 0, SEEK_END);
                sprintf(message, "%020d,%020d;", lastProcess, end);
                write(logidx, message, strlen(message));
                _exit(0);
            }
            else {
                pids[lastProcess] = pid;    
            }
            lastProcess++;
        }
        else if(strncmp(buffer, "exit", 4) == 0) {
            break;
        }
        else if(strncmp(buffer, "listar", 6) == 0) {
            char message[1024];
            int empty = 1;
            for(size_t i = 0; i < lastProcess; i++) {
                if(exitStatus[i] == EXECUTING) {
                    empty = 0;
                    sprintf(message, "#%zu: %s\n", i + 1, processes[i]);
                    write(server_client_fifo, message, strlen(message));
                }
            }
            if(empty) {
                strcpy(message, "Não há tarefas em execução\n");
                write(server_client_fifo, message, strlen(message));
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
                free(status);
            }
            if(lastProcess == 0) {
                strcpy(message, "Histórico vazio\n");
                write(server_client_fifo, message, strlen(message));
            }
        }
        else if(strncmp(buffer, "output", 6) == 0) {
            long num = strtol(buffer + 7, NULL, 10);
            lseek(logidx, 0, SEEK_SET);
            char buf[64];
            long start = 0, procNum = 0, end = 0;
            while(read(logidx, buf, 42) > 0) {
                sscanf(buf, "%ld,%ld;", &procNum, &end);
                if(procNum == num) {
                    long N = end - start;
                    char output[N];
                    lseek(log, start, SEEK_SET);
                    read(log, output, N);
                    write(server_client_fifo, output, N);
                    break;
                }
                start = end;
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