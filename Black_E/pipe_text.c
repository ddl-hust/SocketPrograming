#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
//父子进程通过pipe通信
int main()
{
    int pipefd[2];
    pid_t pid;
    pipe(pipefd);
    pid = fork();
    if (pid == -1)
    {
        perror("fork error");
        exit(1);
    }
    else if (pid > 0)
    {
        close(pipefd[0]);
        char *buf = "hi child";
        write(pipefd[1], buf, sizeof(buf));
        // sleep(1);
    }
    else
    {
        close(pipefd[1]);
        int read_size;
        char buf[1024];
        // sleep(1);
        read_size = read(pipefd[0], buf, sizeof(buf));
        buf[read_size];
        printf("size of buf :%ld\n", sizeof(buf));
        if (read_size == -1)
        {
            perror("read error");
            exit(1);
        }
        else
        {
            write(STDOUT_FILENO, buf, read_size);
        }
    }
    return 0;
}
//问题：
/**
 * 输出：hi child%   这个%哪来的?
 */