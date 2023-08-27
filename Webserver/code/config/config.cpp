#include"config.h"
Config::Config()
{
    //端口号
    m_port=9006;
    //触发组合模式，默认listenfd ET+connfd ET
    m_trigmode=3;
    //优雅关闭连接，默认不使用
    m_opt_linger=0;
    //数据库连接池数量，默认是8
    m_sqlnum=8;
    //线程池的线程数量，默认是8
    m_threadnum=8;
    //日志等级，默认是等级1
    m_loglevel=1;
    //日志写入方式，默认是异步
    m_logwrite=1;
}
void Config::parase_arg(int argc,char* argv[])
{
    int opt;
    const char* str="p:m:o:s:t:l:g:";
    while((opt=getopt(argc,argv,str))!=-1)
    {
        switch(opt)
        {
            case 'p':
            m_port=atoi(optarg);
            break;
            break;
            case 'm':
            m_trigmode=atoi(optarg);
            break;
            case 'o':
            m_opt_linger=atoi(optarg);
            break;
            case 's':
            m_sqlnum=atoi(optarg);
            break;
            case 't':
            m_threadnum=atoi(optarg);
            break;
            break;
            case 'l':
            m_logwrite=atoi(optarg);
            break;
            case 'g':
            m_loglevel=atoi(optarg);
            break;
            default:
            break;
        }
    }
}