#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

//输出　ps -aux 命令重定向到文件out

extern char **environ;
int main()
{
    int fd = open("out",O_CREAT|O_TRUNC|O_WRONLY);
    dup2(fd, STDOUT_FILENO);
    execlp("ps", "ps", "-aux", NULL);
    return 0;
}