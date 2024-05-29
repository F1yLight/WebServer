1. make: *** [makefile:8: run] 段错误 (核心已转储)
2. 检测url里的“？”是为了判断请求是静态还是动态
3. m_file_stat.st_mode & S_IROTH 是一个位操作，用于检查文件的 "其他用户"（other）是否有读取（read）权限。
4. if (S_ISDIR(m_file_stat.st_mode))
            return BAD_REQUESTION;
S_ISDIR 是一个宏，用于检查文件的类型。如果 m_file_stat.st_mode 表示的文件是一个目录，那么 S_ISDIR(m_file_stat.st_mode) 就会返回真（非零值）。
5. 聚合‘sockaddr_in addr’ 类型不完全
缺省头文件:
netinet/in.h
sys/socket.h
sys/types.h
