#include "../inc/HTTP.h"
#include "../inc/ThreadPool.h"
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

const int port = 8888;

// 设置非阻塞
void set_nonblocking(int fd) {
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
}

void addfd(int epfd, int fd, bool oneshot) {
    epoll_event ev;
    ev.data.fd = fd;
    // EPOLLIN 表示 fd 上有数据可读，EPOLLET 表示使用边缘触发模式，EPOLLRDHUP 表示对端关闭连接或者关闭写操作。
    ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    if (oneshot)
        ev.events = ev.events | EPOLLONESHOT;
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    set_nonblocking(fd);
}

int main() {
    ThreadPool<HTTP> pool;
    std::vector<HTTP> users(128);
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET; // IPV4
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htons(INADDR_ANY);

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        std::cout << "socket创建失败" << std::endl;
        return 0;
    }

    if (bind(listenfd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        std::cout << "bind失败" << std::endl;
        return 0;
    }

    if (listen(listenfd, 5) < 0) {
        std::cout << "listen失败" << std::endl;
        return 0;
    }

    epoll_event events[1024];
    int epfd = epoll_create(5);
    if (epfd == -1) {
        std::cout << "epoll创建失败" << std::endl;
        return 0;
    }

    // listen不能注册EPOLL ONESHOT事件，否则只能处理一个客户连接
    addfd(epfd, listenfd, 0);
    while (true) {
        int number = epoll_wait(epfd, events, 1024, -1);
        if (number < 0 && errno != EINTR) {
            std::cout << "epoll等待错误" << std::endl;
            break;
        }
        for (int i = 0; i < number; i++) {
            int sockfd = events[i].data.fd;
            if (sockfd == listenfd) {
                struct sockaddr_in client_address;
                socklen_t client_addresslength = sizeof(client_address);
                int client_fd =
                    accept(listenfd, (struct sockaddr *)&client_address,
                           &client_addresslength);
                if (client_fd < 0) {
                    std::cout << "errno is " << errno << std::endl;
                    continue;
                }
                addfd(epfd, client_fd, true);
                std::cout << "client_fd:" << client_fd << std::endl;
                users[client_fd].init(epfd, client_fd);
            } else if (events[i].events &
                       (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) { // 有异常
                users[sockfd].http_close();
            } else if (events[i].events & EPOLLIN) { // 可读取
                if (users[sockfd].http_read())
                    pool.add_task(&users[sockfd]);
                else
                    users[sockfd].http_close();
            } else if (events[i].events & EPOLLOUT) { // 可写入
                if (!users[sockfd].http_write())
                    users[sockfd].http_close();
            }
        }
    }
    close(epfd);
    close(listenfd);

    return 0;
}