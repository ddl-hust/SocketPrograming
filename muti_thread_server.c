#include "wrap.h"
#include <arpa/inet.h>
#include <ctype.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#define NET_PORT 8888

typedef struct thread_info
{
    struct sockaddr_in *client_address;
    int connet_fd;
} thread_info;
void *do_work(void *info)
{
    char buf[BUFSIZ], client_ip[BUFSIZ];
    size_t read_size;
    thread_info *client_info = info;
    int connfd = client_info->connet_fd;
    struct sockaddr_in *client_addr = (client_info->client_address);
    printf("clinet IP:%s, port :%d\n",
           inet_ntop(AF_INET, &client_addr->sin_addr.s_addr, &client_ip, sizeof(client_ip)),
           ntohs(client_addr->sin_port));
    while (1)
    {
        read_size = read(connfd, buf, BUFSIZ);
        if (read_size < 0)
            perr_exit("read error");
        else if (read_size == 0)
            exit(1);
        else
        {
            for (size_t i = 0; i < read_size; i++)
            {
                buf[i] = toupper(buf[i]);
            }
            Write(connfd, buf, read_size);
        }
    }
    close(connfd);
    return NULL;
}
int main()
{
    int lfd, cfd;
    struct sockaddr_in ser_addr, client_addr;

    char buf[BUFSIZ], client_ip[BUFSIZ]; //bufsiz 默认bufsize
    int read_size;
    socklen_t client_addr_size = sizeof(client_addr);
    pthread_t pid;
    thread_info ti[100];
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
    int i = 0;
    while (1)
    {

        cfd = Accept(lfd, (struct sockaddr *)&client_addr, &client_addr_size);
        ti[i].connet_fd = cfd;
        ti[i].client_address = (struct sockaddr_in *)&client_addr;
        pthread_create(&pid, NULL, (void *)do_work, (void *)&ti[i]);
        pthread_detach(pid);
        i++;
    }
    return 0;
}