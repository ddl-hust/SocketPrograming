#include "wrap.h"
#include <arpa/inet.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

#define NET_PORT 8888
int main()
{
    int lfd, cfd;
    struct sockaddr_in ser_addr, client_addr;

    char buf[BUFSIZ]; //bufsiz 默认bufsize
    int read_size;
    socklen_t client_addr_size = sizeof(client_addr);
    pid_t pid;

    lfd = Socket(AF_INET, SOCK_STREAM, 0); //build socket
    bzero(&ser_addr, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_port = htons(NET_PORT); //端口号 可以通过define
    ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    Bind(lfd, (struct sockaddr *)&ser_addr, sizeof(ser_addr));
    Listen(lfd, 100);

    while (1)
    {
        cfd = Accept(lfd, (struct sockaddr *)&client_addr, &client_addr_size);
        //child fork
        pid = fork();
        if (pid < 0)
        {
            perror("fork wrror");
            exit(1);
        }
        else if (pid == 0)
        {
            Close(lfd);
            break;
        }
        else
        {
            Close(cfd);
            // to-do deal dead process
        }
    }

    if (pid == 0)
    {
        while (1)
        {
            read_size = Read(cfd, buf, sizeof(buf));
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
                Write(cfd, buf, sizeof(read_size));
            }
        }
    }

    return 0;
}