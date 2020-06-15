#define MESSAGESIZE 2048

#define FINISHED 0
#define EXECUTING 1
#define TERMINATED 2
#define TERMINACTIVE 3
#define TERMTEXEC 4
#define ERROR 255

void strcpyandtrim(char* dest, char* src);

void terminate(int procNum);