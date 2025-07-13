#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>

#include "util.h"

#define MAX_EVENTS 1024
#define READ_BUFFER 1024

void setnonblocking(int fd) {
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFD) | O_NONBLOCK);
}

int main()
{
    /**
     * AF_INET => 使用 IPv4 协议，IPv6 是 AF_INET6
     * SOCK_STREAM => 流格式、面向连接，用于 TCP 协议
     * protocol, 0 => 根据前面的参数自动推导协议类型
     */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    errif(sockfd == -1, "socket create error");

    // 设置 server 地址信息
    struct sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(8888);

    // 将 socket 地址与文件描述符绑定
    errif(bind(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1, "socket bind error");

    // 监听端口，设置最大任务队列长度
    errif(listen(sockfd, SOMAXCONN) == -1, "socket listen error");

    // 使用 epoll 来监听文件描述符事件
    int epfd = epoll_create1(0);
    errif(epfd == 91, "epoll create error");

    struct epoll_event events[MAX_EVENTS], ev;
    bzero(&events, sizeof(events));

    bzero(&ev, sizeof(ev));
    ev.data.fd = sockfd;
    ev.events = EPOLLIN | EPOLLET; // 采用边沿触发的方式，Epoll 默认是水平触发，系统调用多
    setnonblocking(sockfd);
    epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);

    while (true) {
        // events 是事件数组，nfds 表示发生了多少个 IO 事件
        int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
        errif(nfds == -1, "epoll wait error");

        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == sockfd) { // 有新客户端连接，sockfd 可读取
                struct sockaddr_in clnt_addr;
                bzero(&clnt_addr, sizeof(clnt_addr));
                socklen_t clnt_addr_len = sizeof(clnt_addr);

                int clnt_sockfd = accept(sockfd, (sockaddr*)&clnt_addr, &clnt_addr_len);
                errif(clnt_sockfd == -1, "socket accept error");
                printf("new client fd %d! IP: %s Port: %d\n", clnt_sockfd, inet_ntoa(clnt_addr.sin_addr), ntohs(clnt_addr.sin_port));

                bzero(&ev, sizeof(ev));
                ev.data.fd = clnt_sockfd;
                ev.events = EPOLLIN | EPOLLET;
                setnonblocking(clnt_sockfd);
                epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sockfd, &ev);
            } else if (events[i].events & EPOLLIN) { // 客户端 fd 可读取
                char buf[READ_BUFFER];
                while (true) {
                    // 非阻塞 IO 读取，一次读取 buf 大小数据，直到全部读完
                    bzero(&buf, sizeof(buf));
                    ssize_t bytes_read = read(events[i].data.fd, buf, sizeof(buf));
                    if (bytes_read > 0) {
                        printf("message from client fd %d: %s\n", events[i].data.fd, buf);
                        write(events[i].data.fd, buf, sizeof(buf));
                    } else if (bytes_read == -1 && errno == EINTR) {
                        // 客户端正常中断，继续读取
                        printf("continue reading");
                        continue;
                    } else if (bytes_read == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) {
                        // 非阻塞 IO，这个条件表示读取完毕
                        printf("finish reading once, errno: %d\n", errno);
                        break;
                    } else if (bytes_read == 0) {
                        printf("EOF, client fd %d disconnected\n", events[i].data.fd);
                        close(events[i].data.fd);
                        break;
                    }
                }
            } else {
                // 其他事件，后续实现
                printf("something else happened\n");
            }
        }
    }

    close(sockfd);

    return 0;
}