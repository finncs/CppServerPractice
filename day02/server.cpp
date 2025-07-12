#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "util.h"

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

    // 接收客户端连接
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    bzero(&client_addr, sizeof(client_addr));

    int client_sockfd = accept(sockfd, (sockaddr*)&client_addr, &client_addr_len);
    errif(client_sockfd == -1, "socket accept error");

    printf("new client fd %d! IP: %s Port: %d\n", client_sockfd, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    while (true) {
        char buf[1024];             // 定义缓冲区
        bzero(&buf, sizeof(buf));   // 清空缓冲区
        ssize_t read_bytes = read(client_sockfd, buf, sizeof(buf));
        if (read_bytes > 0) {
            printf("message from client fd %d: %s\n", client_sockfd, buf);
            write(client_sockfd, buf, sizeof(buf));
        } else if (read_bytes == 0) {   // EOF
            printf("client fd %d disconnected\n", client_sockfd);
            close(client_sockfd);
            break;
        } else if (read_bytes == -1) {
            close(client_sockfd);
            errif(true, "socket read error");
        }
    }

    close(sockfd);
    return 0;
}