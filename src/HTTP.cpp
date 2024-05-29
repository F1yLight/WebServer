#ifndef HTTP_H
#include "../inc/HTTP.h"
#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

HTTP::HTTP() {
    epfd = -1;
    fd = -1;
    bzero(read_buf, sizeof(read_buf));
    have_read = 0;
    method = -1;
    bzero(url, sizeof(url));
    bzero(version, sizeof(version));
    keep_alive = 0;
    content_length = -1;
    bzero(host, sizeof(host));
    IS_DYNAMIC = 0;
    bzero(file_path, sizeof(file_path));
    file_size = 0;
    bzero(res_head, sizeof(res_head));
    bzero(res_body, sizeof(res_body));
}

HTTP::~HTTP() {}

void HTTP::init(int _epfd, int _fd) {
    epfd = _epfd;
    fd = _fd;
}

// 读取请求到read_buf中
bool HTTP::http_read() {
    char *read_now = read_buf + have_read;
    while (recv(fd, read_now, sizeof(read_now), 0) == -1) {
        // 如果错误是EAGAIN或EWOULDBLOCK，再次尝试读取
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            continue;
        return 0;
    }
    have_read += strlen(read_now);
    return 1;
}

// 根据 \\r\\n 找到当前行右边界，找不到的话返回-1
int HTTP::find_r(int idx) {
    for (int i = idx, len = strlen(read_buf); i < len - 1; ++i)
        if (read_buf[i] == '\r' && read_buf[i + 1] == '\n')
            return i + 1;
    return -1;
}

// 解析请求行
HTTP::HTTPCode HTTP::parse_request_line(int l, int r) {
    r -= 2;
    // 处理请求方法
    if (strncmp(read_buf + l, "GET", 3) == 0) {
        method = GET;
        l += 4;
    } else if (strncmp(read_buf + l, "POST", 4) == 0) {
        method = POST;
        l += 5;
    } else {
        std::cout << "请求方法错误" << std::endl;
        return RQST_ERR;
    }

    // 处理请求url
    int url_r = l;
    while (url_r <= r && read_buf[url_r] != ' ')
        ++url_r;
    if (url_r > r) {
        std::cout << "请求url错误" << std::endl;
        return RQST_ERR;
    }
    strncpy(url, read_buf + l, url_r - l);

    // 处理请求版本
    l = url_r + 1;
    if (l == r) {
        std::cout << "请求版本缺失" << std::endl;
        return RQST_ERR;
    }
    strncpy(version, read_buf + l, r - l);

    return RQST_RGT;
}

// 解析请求头
HTTP::HTTPCode HTTP::parse_request_header(int l, int r) {
    if (l == r - 1)
        return RQST_RGT;
    r -= 2;
    if (strncasecmp(read_buf + l, "connection:", 11) == 0) {
        l += 11;
        while (read_buf[l] == ' ')
            ++l;
        if (strncasecmp(read_buf + l, "keep-alive", 10) == 0)
            keep_alive = 1;
    } else if (strncasecmp(read_buf + l, "content-length:", 15) == 0) {
        l += 15;
        while (read_buf[l] == ' ')
            ++l;
        char tmp[16];
        strncpy(tmp, read_buf + l, r - l + 1);
        content_length = atoi(tmp);
    } else if (strncasecmp(read_buf + l, "host:", 5) == 0) {
        l += 5;
        while (read_buf[l] == ' ')
            ++l;
        strncpy(host, read_buf + l, r - l + 1);
    } else {
        std::cout << "请求头错误" << std::endl;
        return RQST_ERR;
    }
    return RQST_RGT;
}

// 解析请求
HTTP::HTTPCode HTTP::parse() {
    int l = 0, r = find_r(l);
    if (r == -1)
        return RQST_ING;
    if (parse_request_line(l, r) == RQST_ERR)
        return RQST_ERR;
    while ((l = r + 1) < (int)strlen(read_buf) && (r = find_r(l)) != -1) {
        if (parse_request_header(l, r) == RQST_ERR)
            return RQST_ERR;
    }
    return RQST_RGT;
}

void HTTP::set_fd(int ev) {
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &event);
}

// GET请求处理
HTTP::HTTPCode HTTP::do_get() {
    if (strchr(url, '?')) {
        IS_DYNAMIC = 1;
        return DYNAMIC;
    }
    char path[64] = "web";
    strcpy(file_path, path);
    strcat(file_path, url);
    struct stat st;
    std::cout << file_path << std::endl;
    if (stat(file_path, &st) < 0)
        return ERR_404;

    if (!(st.st_mode & S_IROTH))
        return ERR_403;

    if (S_ISDIR(st.st_mode))
        return ERR_400;

    file_size = st.st_size;

    return GET;
}

// POST请求处理
HTTP::HTTPCode HTTP::do_post() {
    if (content_length < 0)
        return ERR_400;
    char path[64] = "web";
    strcpy(file_path, path);
    strcat(file_path, url);

    if (fork() == 0) {
        dup2(fd, STDOUT_FILENO);
        execl(file_path, read_buf + (strlen(read_buf) - content_length), NULL);
    }
    wait(NULL);
    return POST;
}

// 动态请求响应
void HTTP::dynamic() {
    std::cout << "动态请求" << std::endl;
    int a, b, c = -1;
    char argv[64];
    strcpy(argv, strchr(url, '?') + 1);
    sscanf(argv, "a=%d&b=%d", &a, &b);
    char tmp_body[] =
        "<html><body>\r\n<p>%d %c %d = %d </p><hr>\r\n</body></html>\r\n";
    char tmp_head[] =
        "HTTP/1.1 200 ok\r\nConnection: close\r\ncontent-length: %d\r\n\r\n";
    char opeartor = '?';
    auto f = [](char a[], const char b[]) {
        return strncasecmp(a, b, strlen(b));
    };
    if (f(url, "/add") == 0)
        c = a + b, opeartor = '+';
    else if (f(url, "/sub") == 0)
        c = a - b, opeartor = '-';
    else if (f(url, "/mul") == 0)
        c = a * b, opeartor = '*';
    else if (f(url, "/div") == 0)
        c = a / b, opeartor = '/';
    else if (f(url, "/mod") == 0)
        c = a % b, opeartor = '%';
    else {
        std::cout << "计算格式错误" << std::endl;
        return;
    }
    sprintf(res_body, tmp_body, a, opeartor, b, c);
    sprintf(res_head, tmp_head, strlen(res_body));
}

// 请求错误响应 400
void HTTP::res_400() {
    std::cout << "ERR 400: 请求错误" << std::endl;
    strcpy(file_path, "web/err_400.html");
    struct stat st;
    stat(file_path, &st);
    file_size = st.st_size;
    char res[] = "HTTP/1.1 400 Bad Request\r\nContent-Length: %d\r\n\r\n";
    sprintf(res_head, res, file_size);
}

// 无权限响应 403
void HTTP::res_403() {
    std::cout << "ERR 403: 权限不足" << std::endl;
    strcpy(file_path, "web/err_403.html");
    struct stat st;
    stat(file_path, &st);
    file_size = st.st_size;
    char res[] = "HTTP/1.1 403 Forbidden\r\nContent-Length: %d\r\n\r\n";
    sprintf(res_head, res, file_size);
}
// 找不到文件响应 404
void HTTP::res_404() {
    std::cout << "ERR 404: 文件不存在" << std::endl;
    strcpy(file_path, "web/err_404.html");
    struct stat st;
    stat(file_path, &st);
    file_size = st.st_size;
    char res[] = "HTTP/1.1 404 Not Found\r\nContent-Length: %d\r\n\r\n";
    sprintf(res_head, res, file_size);
}

// GET请求响应 200
void HTTP::res_get() {
    std::cout << "GET请求" << std::endl;
    sprintf(res_head,
            "HTTP/1.1 200 ok\r\nConnection: close\r\ncontent-length:%d\r\n\r\n",
            file_size);
}

// 线程接口函数
void HTTP::doit() {
    int par = parse();
    if (par == RQST_ERR)
        return;
    else if (par == RQST_ING) {
        set_fd(EPOLLIN);
        return;
    }
    int res = RQST_ERR;
    if (method == GET)
        res = do_get();
    else
        res = do_post();

    if (res == DYNAMIC)
        dynamic();
    else if (res == ERR_400)
        res_400();
    else if (res == ERR_403)
        res_403();
    else if (res == ERR_404)
        res_404();
    else if (res == GET)
        res_get();
    else {
        std::cout << "不知名错误" << std::endl;
        return;
    }
    set_fd(EPOLLOUT);
}

bool HTTP::http_write() {
    if (IS_DYNAMIC) {
        if (send(fd, res_head, strlen(res_head), 0) < 0) {
            std::cout << "动态-响应头发送失败" << std::endl;
            return 0;
        }
        if (send(fd, res_body, strlen(res_body), 0) < 0) {
            std::cout << "动态-响应体发送失败" << std::endl;
            return 0;
        }
        return 1;
    }
    std::cout << file_path << std::endl;
    int res_fd = open(file_path, O_RDONLY);
    if (res_fd == -1) {
        close(res_fd);
        std::cout << "文件打开失败" << std::endl;
        return 0;
    }
    if (write(fd, res_head, strlen(res_head)) < 0) {
        close(res_fd);
        std::cout << "响应头发送失败" << std::endl;
        return 0;
    }
    if (sendfile(fd, res_fd, NULL, file_size) < 0) {
        close(res_fd);
        std::cout << "响应体发送失败" << std::endl;
        return 0;
    }
    close(res_fd);
    return 1;
}

void HTTP::http_close() {
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
    fd = -1;
    have_read = 0;
}

#endif

// int main() {
//     HTTP http;
//     strcpy(http.read_buf, "GET /sum.html "
//                           "HTTP/"
//                           "1.1\r\nConnection:keep-alive\r\nContent-Length:"
//                           "10\r\nHost:abcd\r\n\r\n0123456789");
//     http.doit();
//     http.http_write();
//     http.http_close();
//     return 0;
// }
