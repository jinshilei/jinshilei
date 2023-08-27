#ifndef CONFIG_H
#define CONFIG_H

#include"../server/webserver.h"

class Config
{
    public:
    void parase_arg(int argc,char* argv[]);
    Config();
    ~Config(){};
    //端口号
    int m_port;
    //触发组合模式
    int m_trigmode;
    //优雅关闭连接
    int m_opt_linger;
    //数据库连接池数量
    int m_sqlnum;
    //线程池的线程数量
    int m_threadnum;
    //日志等级
    int m_loglevel;
    //日志写入方式,0是同步，1是异步
    int m_logwrite;
};
#endif