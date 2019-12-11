#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
// 循环创建子进程
int main(){
    pid_t pid;
    int i;
    printf("DDDDDDDDDDDD\n");
    for ( i = 0; i < 5; i++)
    {
        pid=fork();
        if(pid==0) break;
    }
    sleep(i);
    if(i<5) printf("I am the %dth child process %d and my parent process is %d\n",i+1,getpid(),getppid());
    else printf("I am the parent process %d\n",getpid());
    return 0;
}