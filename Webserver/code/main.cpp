#include"config/config.h"

int main(int argc,char* argv[])
{
    //数据库配置
    const char* user="root";
    const char* passwd="123456";
    const char* databasename="webdb";
    //解析命令参数
    Config config;
    config.parase_arg(argc,argv);
    //初始化server
    Webserver server(config.m_port,config.m_trigmode,60000,config.m_opt_linger,user,passwd,databasename,config.m_sqlnum,config.m_threadnum,config.m_loglevel,config.m_logwrite);
    //服务器启动
    server.Start();
    return 0;
}