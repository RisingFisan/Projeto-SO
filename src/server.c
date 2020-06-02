#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>

int main(int argc, char const *argv[]) {
    mkfifo("/tmp/fifo", 0644);
    int fifo = open("/tmp/fifo", O_RDONLY);
    char buffer[1024];
    int bytesRead = 0;
    while(1) {
        int bytesRead = read(fifo, buffer, 1024);
        write(STDOUT_FILENO, buffer, bytesRead);
    }
    return 0;
}
