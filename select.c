#include "wrap.h"
#include <arpa/inet.h>
#include <ctype.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <unistd.h>
#define SERV_PORT 8888
//有问题的select函数
int main()
{
    int fd_num, client[FD_SETSIZE]; //
    int maxfd, listenfd, connfd, sockfd;
    char buf[BUFSIZ], str[INET_ADDRSTRLEN], client_ip[BUFSIZ]; //缓冲区
    fd_set rset, allset;                                       //store origin

    // route
    struct sockaddr_in client_addr, serv_addr;
    socklen_t client_addr_size;
    listenfd = Socket(AF_INET, SOCK_STREAM, 0); //build socket

    memset(&serv_addr,0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT); //端口号 可以通过define
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    Bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    Listen(listenfd, 1024);

    maxfd = listenfd; //==3 for [0,1,2]stdin,out,err

    FD_ZERO(&allset);
    FD_SET(maxfd, &allset);
    printf("!!!!!!!!!!!!!!\n");
    while (1)
    {
        rset = allset;
        printf("!!!!!!!!!!!!!!\n");
        if ((fd_num = select(maxfd + 1, &rset, 0, 0, 0)) == -1)
            break; //error
        // printf("!!!!!!!!!!!!!!\n");
        // if (fd_num == 0)
        //     continue; //超时返回值，这里我们没有设置时间
        // printf("!!!!!!!!!!!!!!\n");
        for (int i = 0; i < maxfd + 1; i++)
        {
            if (FD_ISSET(i, &rset))
            {
                if (i == listenfd)
                { //监听连接
                    client_addr_size = sizeof(client_addr);
                    printf("!!!!!!!!!!!!!!\n");
                    connfd = Accept(listenfd, (struct sockaddr *)&client_addr, &client_addr_size);
                    printf("clinet IP:%s, port :%d\n",
                           inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, client_ip, sizeof(client_ip)),
                           ntohs(client_addr.sin_port));
                    FD_SET(connfd, &allset);
                    maxfd = connfd > maxfd ? connfd : maxfd;
                }
                else
                { //读消息
                    printf("!!!!!!!!!!!!!!\n");
                    int str_len = Read(i, buf, BUFSIZ);
                    if (str_len == 0)
                    {
                        FD_CLR(i, &allset);
                        Close(i);
                        printf("close clinet :%d", i);
                    }
                    else
                    {
                        for (int i = 0; i < str_len; i++)
                            buf[i] = toupper(buf[i]);
                        Write(STDOUT_FILENO, buf, str_len);
                        Write(i, buf, str_len);
                    }
                }
            }
        }
    }
    Close(listenfd);
    return 0;
}
