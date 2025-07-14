#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <arpa/inet.h>
#include <unistd.h>
#include "util.h"

#define BUFFER_SIZE 1024

int main()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(8888);

    errif(connect(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1, "socket connect error");

    while (true) {
        char buf[BUFFER_SIZE];      // 在这个版本中，缓冲区的大小为什么一定要和 server 端相同？
        bzero(&buf, sizeof(buf));   // 清空缓冲区
        scanf("%s", buf);
        ssize_t write_bytes = write(sockfd, buf, sizeof(buf));
        if (write_bytes == -1) {
            printf("socket already disconnected, con't write any more!\n");
            break;
        }

        bzero(&buf, sizeof(buf));
        ssize_t read_bytes = read(sockfd, buf, sizeof(buf));
        if (read_bytes > 0) {
            printf("message from server: %s\n", buf);
        } else if (read_bytes == 0) {   // EOF
            printf("server socket disconnected\n");
            break;
        } else if (read_bytes == -1) {
            close(sockfd);
            errif(true, "socket read error");
        }
    }
    close(sockfd);
    return 0;
}