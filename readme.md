why duojingcheng 不能支持高并发

- MTU 最大传输单元 协议限制  以太网 1500 IP 65535
- mss 数据包出去协议后，剩下数据大小
- WIN 滑动窗口 当前能接受数据上限值(byte)

-----------
真正的高并发 select poll epoll

1. tcp 状态转换图
   TIME-WAIT 确保最后一个ack 能到达

建立状态图与程序联系
netstat -apn | grep  port_number

先运行服务器 --- 
`tcp        0      0 0.0.0.0:8888            0.0.0.0:*          LISTEN      18297/./server      
`
运行客户端
```
tcp        0      0 127.0.0.1:58500         127.0.0.1:8888          ESTABLISHED 19079/nc            
tcp        0      0 127.0.0.1:8888          127.0.0.1:58500         ESTABLISHED 19080/./server      
```
可以看到此时客户端，服务器状态改变为ESTABLISHED

客户端发起关闭(close+D)
```
客户端：FIN_WAIT2
服务器端： CLOSE_WAIT
```
当你关闭服务器，以及客户端(主动发起关闭一方)此时处于TIME-WAIT等待2msl
如果在此运行服务器，客户端将不能正常通信，此时客户端绑定的端口号还没有释放

上午总结：
关键就是那张TCP状态图，以及与程序之间关系


close() 能半关闭 但是有不足
如果有多个文件描述符指向同一个socket (dup2函数)
此时close()不能全部关闭
通过调用shutdown()

### 端口复用
why:在等待期2msl 此时端口还没有解除绑定，但是可以复用

int setsockopt(int sockfd, int level, int optname,
                      const void *optval, socklen_t optlen);
在绑定之前设置
int opt = 1;
setsockopt(sockfd, SOL_SOCKET,SO_REUSEADDR,
(const void *)&opt, sizeof(opt));
// ~~但是我没有复现这样的错误：
    端口的绑定还在，但是还可以从新开新的服务器？？？~~

### 多路IO转发
> 通过内核来阻塞等待，连接请求，发送数据请求(read 阻塞)
也就是说将那些需要阻塞等待的时间全交给内核，内核发现有客户端请求连接，内核将该信号发给服务端，此时服务端直接处理建立连接
即不用自己的accept()阻塞等待，客户端发送数据也是一样，之前的版本是服务器在read()阻塞等待客户端数据。
`int select(int nfds, fd_set *readfds, fd_set *writefds,
                  fd_set *exceptfds, struct timeval *timeout);`
参数解释：
1. nfds:所监听的文件描述符，最大文件描述符+1
>nfds should be set to the highest-numbered file descriptor in any of the three sets, plus 1.  The indicated file descriptors in each set are checked, up to this limit (but see BUGS).
2. 参数2/3/4 所监听的文件描述符“可读”，“可写”，“异常”监听集合 
3.  struct timeval {
               long    tv_sec;         /* seconds */
               long    tv_usec;        /* microseconds */
           };
返回值：
RETURN VALUE
       On success, select()  return the number of file descriptors contained in the three returned descriptor sets (that is,the total number of bits
       that  are set in readfds, writefds, exceptfds) which may be zero if the timeout expires before anything interesting happens.  On error, -1 is returned, and
       errno is set to indicate the error; the file descriptor sets are unmodified, and timeout becomes undefined.
问题：
    如何添加到文件描述符集合
    如何根据返回值判断每一种集合的个数
通过四个辅助函数：
void FD_CLR(int fd, fd_set *set);
int  FD_ISSET(int fd, fd_set *set); //判断特定文件描述符是否在集合里面
void FD_SET(int fd, fd_set *set);
void FD_ZERO(fd_set *set);

fd_set *readfds传入传出参数：
传入的bit map 可能与传出不一样
因此每个传入的fd 度需要通过FD_ISSET()返回值判断

select问题：
- 同时监听文件描述符上限1024
- 稀疏时候效率底下 eg. fd{1,1023} 根据返回值判断时候for(1->1023)
- 监听集合与满足监听条件集合为同一个集合，需要提前保存原有集合
  
select 监听连接时候对应可读事件

### poll
1. 与select 区别
-  将返回事件，与监听事件分开，存在一个结构体里面，这样就与select有了很大区别
select()里面啊的参数是传入传出  。
- 突破了select()文件描述符的上限1024
- 搜索范围小？？？这个不太理解
还是有问题：如果我监听了1000 fd ,返回3，我还是需要从100个里面便利，不知道是哪个文件描述符状态改变
查看poll()文档
POLLIN 读
POLLOUT 写
POLLERR 错误

到底什么是事件？？？？
阻塞？？？
就绪态？运行态？
网络的知识很多和操作系统是结合在一起的
[https://my.oschina.net/u/138210/blog/59374](https://my.oschina.net/u/138210/blog/59374)


poll()：
int poll(struct pollfd *fds, nfds_t nfds, int timeout);
将timeout ==-1 永久阻塞

-----
星期五
### epoll
pcb-进程控制快 

- int epoll_creat(int size)
返回值 epfd 也是一个文件描述符 指向内核里面一个二叉树树根，该二叉树为平衡二叉树
进一步，该二叉树为红黑树，插入删除效率高
 也就可以理解了为什么需要传入一个size 用来常见树

 - int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);

看下最后一个参数
```
 struct epoll_event {
               uint32_t     events;      /* Epoll events */
               epoll_data_t data;        /* User data variable */
           };


typedef union epoll_data {
               void        *ptr;
               int          fd;
               uint32_t     u32;
               uint64_t     u64;
           } epoll_data_t;

```
这个里面的fd与外面的fd一样，将待监听事件添加到之前创建的红黑树上


- epoll_wait
  
这函数实际对应select,阻塞
int epoll_wait(int epfd, struct epoll_event *events(数组，传出参数 注意与epoll_ctl里面区别),
                      int maxevents, int timeout);

------------
epoll_data 里面的 void* ptr
泛型指针，可以传递任何类型对象
eg.
```
struct 
{   
    int fd;
    void * arg;
    void(*foo)(int fd,void* arg)
}
```
这样威力就很大了

- epoll ET  边沿触发 eg.
- epoll LT 水平触发(默认)
  
边沿触发目的：减少epoll_wait 调用，减少阻塞时间
场景：客户端发送100b,我只读取了500b
内存里面还有500b,此时epoll,触发？

水平触发为默认，只要缓冲区有数据，就会触发
边沿触发： EPOLLET 对方有新事件发生，才会触发
边沿触发应用场景：？

- 设置非阻塞方法：
- 1. fcntl
- 2.open
### epoll 边沿非阻塞触发  while(read()),fcntl(O_NOBLOCK)
好处：减少epoll_wait调用，减少系统开销
回到之前的例子，
场景：客户端发送100b,我只读取了500b
内存里面还有500b，如果我们通过边沿触发的方式来读，显然剩下的500b就还在缓存区，这样可能导致缓冲区数据越来越多
如果我们通过水平触发的方式来读取，这样就会调用2次epoll_wait()
有没有更好的办法？
```
// 获取flags
int flags = fcntl(fd, F_GETFL);
flags = flags|O_NONBLOCK;
fcntl(fd, F_SETFL, flags); //设置待监控的socket为非阻塞

epoll_wait();
while( read(fd,)>0) white();  //这样就能够只调用一次epoll_wait，同时非阻塞读取数据 ！！！
```

**以上，边沿非阻塞效率最高，且能满足我们需求**


## libevent

epoll_wait() 返回调用回调函数？





