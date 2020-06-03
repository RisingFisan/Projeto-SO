#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

void strcpyandtrim(char* dest, char* src);

int main(int argc, char const *argv[]) {
    mkfifo("./fifo", 0644);
    int fifo = open("./fifo", O_RDONLY);
    ssize_t bytesRead = 0;
    while(1) {
        char buffer[1024];
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
            char command[1024];
            strcpyandtrim(command, buffer + 9);
            write(STDOUT_FILENO, command, strlen(command));
            write(STDOUT_FILENO, "\n", 1);
        }
        else if(strncmp(buffer, "exit", 4) == 0) {
            break;
        }
        else write(STDOUT_FILENO, buffer, bytesRead);
    }
    return 0;
}

void strcpyandtrim(char* dest, char* src) {
    char* start = src;
    while(*start == ' ') start++;
    start++;
    strcpy(dest, start);
    while(dest[strlen(dest) - 1] == ' ') dest[strlen(dest) - 1] = '\0';
    dest[strlen(dest) - 1] = '\0';
}