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
int execTimes[1024];
char commTimes[1024];
int pids[1024][32];
int numPids[1024];
int lastProcess = 0;
int tExec = -1;
int tInac = -1;

void strcpyandtrim(char* dest, char* src);

void terminate(int procNum);

void sigchld_handler(int sig) {
    int status;
    pid_t pid = wait(&status);
    for(size_t i = 0; i < lastProcess; i++) {
        if(pids[i][numPids[i] - 1] == pid) {
            if(!WIFSIGNALED(status) && exitStatus[i] == EXECUTING) {
                int log = open("log", O_RDONLY);
                int logidx = open("log.idx", O_WRONLY | O_APPEND);
                exitStatus[i] = FINISHED;
                off_t end = lseek(log, 0, SEEK_END);
                char message[64];
                sprintf(message, "%020zu,%020ld;", i, end);
                write(logidx, message, strlen(message));
                break;
            }
        }
    }
}

void sigalrm_handler(int sig) {
    for(size_t procNum = 0; procNum < lastProcess; procNum++) {
        if(exitStatus[procNum] == EXECUTING) {
            execTimes[procNum]++;
            commTimes[procNum]++;
            if(tExec > 0 && execTimes[procNum] >= tExec) {
                terminate(procNum);
                exitStatus[procNum] = TERMTEXEC;
            }
            else if(tInac > 0 && commTimes[procNum] >= tInac) {
                terminate(procNum);
                exitStatus[procNum] = TERMINACTIVE;
            }
        }
    }
    alarm(1);
}

void sigio_handler(int sig) {
    int pid = getpid();
    for(size_t i = 0; i < lastProcess; i++) {
        for(size_t j = 0; j < numPids[i] - 1; j++) {
            if(pid == pids[i][j]) {
                commTimes[i] = 0;
                return;
            }
        }
    }
}

int main(int argc, char const *argv[]) {
    mkfifo("client_server_fifo", 0644);
    mkfifo("server_client_fifo", 0644);

    signal(SIGCHLD, sigchld_handler);
    signal(SIGALRM, sigalrm_handler);
    signal(SIGIO, sigio_handler);

    int log = open("log", O_RDWR | O_CREAT | O_TRUNC, 0644);
    int logidx = open("log.idx", O_RDWR | O_CREAT | O_TRUNC, 0644);

    alarm(1);

    while(1) {
        char* buffer = calloc(1024, sizeof(char));
        int client_server_fifo = open("client_server_fifo", O_RDONLY);
        int server_client_fifo = open("server_client_fifo", O_WRONLY);
        size_t bytesRead = read(client_server_fifo, buffer, 1024);

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
            close(server_client_fifo);
        }
        else if(strncmp(buffer, "executar", 8) == 0) {
            char* command = calloc(1024, sizeof(char));
            strcpyandtrim(command, buffer + 9);

            processes[lastProcess] = strdup(command);
            exitStatus[lastProcess] = EXECUTING;

            char message[64];
            sprintf(message, "nova tarefa #%d\n", lastProcess);
            write(server_client_fifo, message, strlen(message));

            int pid;
            int pipes[32][2];
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
                    fcntl(pipes[currPipe][0], __F_SETOWN, getpid());
                    fcntl(pipes[currPipe][0], F_SETFL, O_ASYNC | O_RDONLY);

                    args[i] = NULL;
                    if((pid = fork()) == 0) {
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
                        pids[lastProcess][currPipe] = pid;
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
            if((pid = fork()) == 0) {
                dup2(log, STDOUT_FILENO);
                close(log);
                if(currPipe != 0) {
                    dup2(pipes[currPipe - 1][0], STDIN_FILENO);
                    close(pipes[currPipe - 1][0]);
                }
                execvp(args[0], args + 1);
            }
            else pids[lastProcess][currPipe] = pid;
            numPids[lastProcess] = currPipe + 1;

            lastProcess++;
        }
        else if(strncmp(buffer, "terminar", 8) == 0) {
            long num = strtol(buffer + 9, NULL, 10);
            if(num < lastProcess && exitStatus[num] == EXECUTING) {
                terminate(num);
                exitStatus[num] = TERMINATED;
                char message[] = "tarefa terminada com sucesso\n";
                write(server_client_fifo, message, strlen(message));
            }
            else {
                char message[] = "tarefa terminada com sucesso\n";
                write(server_client_fifo, message, strlen(message));
            }
        }
        else if(strncmp(buffer, "exit", 4) == 0) {
            break;
        }
        else if(strncmp(buffer, "tempo-execucao", 14) == 0) {
            long time = strtol(buffer + 15, NULL, 10);
            if(time > 0) {
                tExec = time;
                char message[] = "tempo definido com sucesso\n";
                write(server_client_fifo, message, strlen(message));
            }
            else {
                char message[] = "tempo inválido\n";
                write(server_client_fifo, message, strlen(message));
            }
        }
        else if(strncmp(buffer, "tempo-inactividade", 18) == 0) {
            long time = strtol(buffer + 19, NULL, 10);
            if(time > 0) {
                tInac = time;
                char message[] = "tempo definido com sucesso\n";
                write(server_client_fifo, message, strlen(message));
            }
            else {
                char message[] = "tempo inválido\n";
                write(server_client_fifo, message, strlen(message));
            }
        }
        else if(strncmp(buffer, "listar", 6) == 0) {
            char message[1024];
            int empty = 1;
            for(size_t i = 0; i < lastProcess; i++) {
                if(exitStatus[i] == EXECUTING) {
                    empty = 0;
                    sprintf(message, "#%zu: %s\n", i, processes[i]);
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
                sprintf(message, "#%zu, %s: %s\n", i, status, processes[i]);
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
            int procExists = 0;

            while(read(logidx, buf, 42) > 0) {
                sscanf(buf, "%ld,%ld;", &procNum, &end);
                if(procNum == num) {
                    procExists = 1;
                    long N = end - start;
                    if(N == 0) write(server_client_fifo, "tarefa não produziu output\n", 28);
                    else {
                        char output[N];
                        lseek(log, start, SEEK_SET);
                        read(log, output, N);
                        write(server_client_fifo, output, N);
                    }
                    break;
                }
                start = end;
            }
            if(!procExists) {
                strcpy(buf, "tarefa não encontrada\n");
                write(server_client_fifo, buf, strlen(buf));       
            }
            lseek(log, 0, SEEK_END);
        }
        else write(server_client_fifo, buffer, bytesRead);

        free(buffer);
        close(server_client_fifo);
        close(client_server_fifo);
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

void terminate(int procNum) {
    for(size_t i = 0; i < numPids[procNum]; i++)
        kill(pids[procNum][i], SIGTERM);
}