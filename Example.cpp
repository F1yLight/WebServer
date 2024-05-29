#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

const int port = 8888;

int main(int argc, char *argv[]) {
    if (argc < 0) {
        printf("need two canshu\n");
        return 1;
    }

    //------创建server_address
    // sockaddr_in专门用于处理 Internet 地址的，它包含四个字段，分别是
    // sin_family、sin_port、sin_addr、sin_zero
    // sockaddr可以用于处理任何类型的地址，它只包含两个字段：sa_family（地址族）和
    // sa_data（包含了地址和端口号的14个字节的数组）。
    sockaddr_in sever_address;
    bzero(&sever_address, sizeof(sever_address));

    // sin是sockaddr_in结构体的缩写，sockaddr_in是套接字地址结构，sin_family是地址族，sin_addr是IP地址，sin_port是端口号
    // PF_INET指使用IPv4协议，如果用IPv6则为PF_INET6
    sever_address.sin_family = PF_INET;

    // INADDR_ANY是一个宏，表示本机的所有IP地址
    // htons是一个函数，用于将主机字节序转换为网络字节序
    // s_addr是一个32位的整数，用于存储IP地址
    sever_address.sin_addr.s_addr = htons(INADDR_ANY); // 监听本机所有的IP
    sever_address.sin_port = htons(8888);              // 监听8888端口

    //------创建socket
    // AF指的是地址族，PF指的是协议族
    // AF_INET指使用IPv4协议，如果用IPv6则为AF_INET6
    // SOCK_STREAM指使用TCP协议，如果用UDP则为SOCK_DGRAM
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    //------绑定socket跟server_address
    int ret = bind(sock, (sockaddr *)&sever_address, sizeof(sever_address));
    assert(ret != -1);

    //------开始监听
    ret = listen(sock, 1);
    assert(ret != -1);

    while (1) {
        sockaddr_in client;
        socklen_t client_addrlength = sizeof(client);

        // 如果连接成功，accept返回一个非负的整数，这个整数就是新的套接字描述符。
        int connfd = accept(sock, (sockaddr *)&client, &client_addrlength);
        if (connfd < 0) {
            printf("errno\n");
            continue;
        }
        char request[1024];
        recv(connfd, request, 1024, 0);
        request[strlen(request) + 1] = '\0';
        printf("%s\n", request);
        printf("successeful!\n");
        char buf[520] =
            "HTTP/1.1 200 ok\r\nconnection: close\r\n\r\n"; // HTTP响应
        int s = send(connfd, buf, strlen(buf), 0);          // 发送响应
        // printf("send=%d\n",s);
        int fd = open("hello.html", O_RDONLY); // 消息体
        sendfile(connfd, fd, NULL, 2500);      // 零拷贝发送消息体
        close(fd);
        close(connfd);
    }
    return 0;
}