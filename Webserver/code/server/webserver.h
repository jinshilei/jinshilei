#ifndef WEBSERVER_H
#define WEBSERVER_H

#include<unordered_map>
#include<fcntl.h>
#include<unistd.h>//close
#include<assert.h>
#include<errno.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>

#include"../epoll/epoller.h"
#include"../log/log.h"
#include"../http/httpconn.h"
#include"../timer/heaptimer.h"
#include"../pool/sqlconnRAII.h"
#include"../pool/sqlconnpool.h"
#include"../pool/threadpool.h"

class Webserver 
{
    public:
    ~Webserver();
    //初始化server
    Webserver(int port,int trigmode,int timeoutMS,int optlinger,const char* user,const char* passwd,const char* dbname,int connpoolnum,int threadnnum,int loglevel,int logwrite);
    //服务器开始工作
    void Start();
    private:
    //初始化socket
    bool InitSocket();
    //初始化epoll事件触发模式
    void InitEventMode(int trigmode);
    //添加新的客户连接
    void AddClient(int fd,sockaddr_in addr);
    //处理监听
    void DealListen();
    //处理写
    void DealWrite(HttpConn* client);
    //处理读
    void DealRead(HttpConn* client);
    //发送错误信息
    void SendError(int fd,const char* info);
    //调整定时器事件
    void ExtendTime(HttpConn* client);
    //关闭服务器连接
    void CloseConn(HttpConn* client);
    //正在读
    void OnRead(HttpConn* client);
    //正在写
    void OnWrite(HttpConn* client);
    //在处理服务器运行过程
    void OnProcess(HttpConn* client);
    //设置为非阻塞
    static int SetFdNonblock(int fd);
    private:
    //最大客户连接数
    static const int MAX_FD=65535;
    //端口号
    int m_port;
    //优雅关闭连接
    int m_optlinger;
    //超时时间
    int m_timeoutMS;
    //是否关闭服务
    int m_isclose;
    //监听文件描述符
    int m_listenfd;
    //资源的路径
    char* m_srcDir;
    //epoll事件
    uint32_t m_listenevent;
    uint32_t m_connevent;
    //定时器
    std::unique_ptr<HeapTimer> m_timer;
    //线程池
    std::unique_ptr<ThreadPool> m_threadpool;
    //epoll
    std::unique_ptr<Epoller> m_epoller;
    //http客户连接
    std::unordered_map<int,HttpConn> m_users;
};

#endif //WEBSERVER_H