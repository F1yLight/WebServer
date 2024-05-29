#ifndef HTTP_H
#define HTTP_H

#define BUFFER_SIZE 2048

class HTTP {
    // ~~~~~~~~~~~~~~~~~属性定义~~~~~~~~~~~~~~~~~
  public:
    enum HTTPCode {
        GET,      // 代表GET请求
        POST,     // 代表POST请求
        RQST_ERR, // 代表请求错误
        RQST_RGT, // 代表请求正确进行
        RQST_ING, // 代表请求未完成
        DYNAMIC,  // 代表动态请求
        ERR_400, // 服务器不理解请求的语法（这里是因为路径为目录，而不是文件，无法打开）
        ERR_403, // 其他用户（非服务器本身）无权限访问
        ERR_404  // 找不到文件
    };

    int epfd;                   // epoll描述符
    int fd;                     // 当前使用的文件的描述符
    int have_read;              // 已经读取的字节数
    char read_buf[BUFFER_SIZE]; // 读请求缓冲区
    int method;                 // 请求方法
    char url[256];              // 请求的url
    char version[16];           // 请求的版本
    bool keep_alive;            // 是否保持连接
    int content_length;         // 请求体长度
    char host[64];              // 请求的主机
    bool IS_DYNAMIC;            // 是否为动态请求
    char file_path[256];        // 请求的文件路径
    int file_size;              // 请求的文件大小
    char res_head[BUFFER_SIZE]; // 响应头
    char res_body[BUFFER_SIZE]; // 响应体

    // ~~~~~~~~~~~~~~~~~方法定义~~~~~~~~~~~~~~~~~
  public:
    HTTP();
    ~HTTP();
    void init(int _epfd, int _fd);
    bool http_read();
    HTTPCode parse_request_line(int l, int r);
    HTTPCode parse_request_header(int l, int r);
    int find_r(int idx);
    HTTPCode parse();
    void set_fd(int ev);
    HTTPCode do_get();
    HTTPCode do_post();
    void dynamic();
    void res_400();
    void res_403();
    void res_404();
    void res_get();
    void doit();
    bool http_write();
    void http_close();
};

#endif