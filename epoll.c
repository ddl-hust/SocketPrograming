#include "wrap.h"
#include <arpa/inet.h>
#include <ctype.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <unistd.h>

#define SEVR_PORT 8888

int main()
{
    struct sockaddr_in ser_addr, client_addr;
    int listen_sock; // socket fd
    char buf[BUFSIZ];
    int read_size;
    listen_sock = Socket(AF_INET, SOCK_STREAM, 0);

    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_port = htons(SEVR_PORT);
    ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(opt));

    Bind(listen_sock, (struct sockaddr *)&ser_addr, sizeof(ser_addr));
    Listen(listen_sock, 100);

    /********************/
    int epfd = epoll_create(100);
    struct epoll_event event, events[100];
    event.events = EPOLLIN;
    event.data.fd = listen_sock;
    epoll_ctl(epfd, EPOLL_CTL_ADD, listen_sock, &event);
    while (1)
    {

        int tigger_num = epoll_wait(epfd, events, 100, -1);
        if (tigger_num < 0)
            exit(1);

        for (int i = 0; i < tigger_num; i++)
        {
            int temp = events[i].data.fd;
            if (temp == listen_sock) // 连接请求
            {
                int client_addr_size = sizeof(client_addr);
                int connfd = Accept(listen_sock, (struct sockaddr *)&client_addr, &client_addr_size);
                event.events = EPOLLIN | EPOLLET;
                event.data.fd = connfd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &event);
                char ip[64] = {0};
                printf("New Client IP: %s, Port: %d\n",
                       inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, ip, sizeof(ip)),
                       ntohs(client_addr.sin_port));
            }
            else
            {
                if (!events[i].events & EPOLLIN)
                    continue;
                read_size = Read(temp, buf, BUFSIZ);
                if (read_size == 0)
                {
                    printf("client disconnected ....\n");
                    epoll_ctl(epfd, EPOLL_CTL_DEL, temp, NULL); //注意写法 删除的话不需要在监听
                    Close(temp);
                }
                else
                {
                    for (int i = 0; i < read_size; i++)
                    {
                        buf[i] = toupper(buf[i]);
                    }
                    Write(temp, buf, read_size / 2);
                }
            }
        }
    }
    Close(listen_sock);
    return 0;
}