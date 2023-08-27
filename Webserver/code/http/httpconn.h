#ifndef HTTP_CONN_H
#define HTTP_CONN_H


#include<sys/types.h>
#include<sys/uio.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<errno.h>
#include"../log/log.h"
#include"../pool/sqlconnRAII.h"
#include"../buffer/buffer.h"
#include"httprequest.h"
#include"httpresponse.h"

class HttpConn 
{
    public:
    HttpConn();
    ~HttpConn();
    //初始化连接信息
    void init(int sockfd,const sockaddr_in &addr);
    ssize_t read(int* saveErrno);
    ssize_t write(int* saveErrno);
    //关闭连接
    void Close();
    int GetFd() const;
    int GetPort() const;
    //获取IP
    const char* GetIP() const;
    //获取socket地址
    sockaddr_in GetAddr() const;
    //http请求处理过程
    bool process();
    //获取数据写的长度
    int toWriteBytes()
    {
        return m_iov[0].iov_len+m_iov[1].iov_len;
    }
    //判断请求是否保持连接
    bool isKeepAlive() const
    {
        return m_request.isKeepAlive();
    }
    //epoll是否采用ET模式
    static bool m_isET;
    static const char* m_srcDir;
    //用户连接数
    static std::atomic<int> m_usercount;
    
    private:
    int m_fd;
    struct sockaddr_in m_addr;
    bool m_isclose;
    //分散读
    struct iovec m_iov[2];
    //分散缓冲区个数
    int m_iovcnt;
    //读缓冲区
    Buffer m_readbuff;
    //写缓冲区
    Buffer m_writebuff;
    //处理请求
    HttpRequest m_request;
    //处理响应
    HttpResponse m_response;
};


#endif //HTTP_CONN_H