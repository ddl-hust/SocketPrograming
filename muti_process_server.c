#define _XOPEN_SOURCE 700  //https://stackoverflow.com/questions/6491019/struct-sigaction-incomplete-error
#include "wrap.h"
#include <arpa/inet.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include<sys/wait.h>
#include<signal.h>

void sigproc(int status){
    waitpid(-1,&status,0);
}

#define NET_PORT 8888
int main()
{
    int lfd, cfd;
    int status;
    struct sockaddr_in ser_addr, client_addr;

    char buf[BUFSIZ], client_ip[BUFSIZ]; //bufsiz 默认bufsize
    int read_size;
    socklen_t client_addr_size = sizeof(client_addr);
    pid_t pid;
    struct sigaction act;
    lfd = Socket(AF_INET, SOCK_STREAM, 0); //build socket
    bzero(&ser_addr, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_port = htons(NET_PORT); //端口号 可以通过define
    ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    //端口复用
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(opt));

    Bind(lfd, (struct sockaddr *)&ser_addr, sizeof(ser_addr));
    Listen(lfd, 100);

    while (1)
    {
        cfd = Accept(lfd, (struct sockaddr *)&client_addr, &client_addr_size);
        printf("clinet IP:%s, port :%d\n",
               inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, &client_ip, sizeof(client_ip)),
               ntohs(client_addr.sin_port));
        //child fork
        pid = fork();
        if (pid < 0)
        {
            perror("fork wrror");
            exit(1);
        }
        else if (pid == 0)
        {
            Close(lfd); //子线程不需要做监听任务，释放掉监听socket
            break;
        }
        else
        {   
            memset(&act,0,sizeof(act));
            act.sa_handler = sigproc;
            sigaction(SIGCHLD,&act,0);

            Close(cfd);
            // to-do deal dead process
            // wait(&status);
            // waitpid(-1,&status,WNOHANG);
        }
    }

    if (pid == 0)
    {
        while (1)
        {
            read_size = Read(cfd, buf, 8192); //fix bug buf是一个指针 所以每次都只能读取四个字节
            if (read_size == 0)
            {
                Close(cfd);
                return 0;
            }
            else if (read_size == -1)
            {
                perror("read error"); //singal intur deal in Read()
                exit(1);
            }
            else
            {
                for (int i = 0; i < read_size; i++)
                    buf[i] = toupper(buf[i]);
                Write(STDOUT_FILENO, buf, read_size);
                Write(cfd, buf, read_size); //also a bug
            }
        }
    }

    return 0;
}