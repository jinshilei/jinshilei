#include"webserver.h"

using namespace std;

//初始化sever信息
Webserver::Webserver(int port,int trigmode,int timeoutMS,int optlinger,const char* user,const char* passwd,const char* dbname,int connpoolnum,int threadnum,int loglevel,int logwrite):m_port(port),m_timeoutMS(timeoutMS),m_optlinger(optlinger),m_timer(new HeapTimer()),m_threadpool(new ThreadPool(threadnum)),m_epoller(new Epoller())
{
    //获取当前工作目录路径
    m_srcDir=getcwd(nullptr,256);
    assert(m_srcDir);
    strncat(m_srcDir,"/resources/",16);
    HttpConn::m_usercount=0;
    HttpConn::m_srcDir=m_srcDir;

    //初始化事件触发模式
    InitEventMode(trigmode);
    if(logwrite==1)//异步
    {
        Log::Instance()->init(loglevel,"./log",".log",1024);
    }
    else//同步
    {
        Log::Instance()->init(loglevel,"./log",".log",0);
    }
    //初始化数据库
    SqlConnPool::Instance()->init("localhost",user,passwd,dbname,3306,connpoolnum);
    if(!InitSocket())
    {
        m_isclose=true;
    }
    if(m_isclose)
    {
        LOG_ERROR("server init errror!");
    }
    else
    {
        LOG_INFO("server init!");
        LOG_INFO("Port:%d,OpenLinger:%s",m_port,optlinger?"true":"false");
        LOG_INFO("Listen Mode:%s,OpenConn Mode:%s",(m_listenevent&EPOLLET?"ET":"LT"),(m_connevent&EPOLLET?"ET":"LT"));
        LOG_INFO("Log level:%d",loglevel);
        LOG_INFO("source dircetor:%s",m_srcDir);
        LOG_INFO("SqlConnPool num:%d,ThreadPool num:%d",connpoolnum,threadnum);
    }
}

Webserver::~Webserver()
{
    close(m_listenfd);
    m_isclose=true;
    free(m_srcDir);
    SqlConnPool::Instance()->ClosePool();
}

//初始化epoll事件触发模式，默认ET
void Webserver::InitEventMode(int trigmode)
{
    m_listenevent=EPOLLRDHUP;
    //连接采用epolloneshot
    m_connevent=EPOLLONESHOT|EPOLLRDHUP;
    switch(trigmode)
    {
        case 0:
        break;
        case 1:
        m_connevent|=EPOLLET;
        break;
        case 2:
        m_listenevent|=EPOLLET;
        break;
        case 3:
        m_connevent|=EPOLLET;
        m_listenevent|=EPOLLET;
        break;
        default:
        m_connevent|=EPOLLET;
        m_listenevent|=EPOLLET;
        break;
    }
    HttpConn::m_isET=(m_connevent&EPOLLET);
}

//服务器启动
void Webserver::Start()
{
    //epoll wait timeout=-1无事件将阻塞
    int timeMs=-1;
    if(!m_isclose)
    {
        LOG_INFO("server start");
    }
    while(!m_isclose)
    {
        if(m_timeoutMS>0)
        {
            timeMs=m_timer->GetNextTick();
        }
        //等待epoll事件
        int eventcnt=m_epoller->Wait(timeMs);
        //处理所有epoll事件
        for(int i=0;i<eventcnt;i++)
        {
            int fd=m_epoller->GetEventfd(i);
            uint32_t events=m_epoller->GetEvents(i);
            //处理新的链接
            if(fd==m_listenfd)
            {
                DealListen();
            }
            //服务器关闭链接
            else if(events&(EPOLLRDHUP|EPOLLHUP|EPOLLERR))
            {
                assert(m_users.count(fd)>0);
                CloseConn(&m_users[fd]);
            }
            //处理收到的数据
            else if(events&EPOLLIN)
            {
                assert(m_users.count(fd)>0);
                DealRead(&m_users[fd]);
            }
            //处理写数据
            else if(events&EPOLLOUT)
            {
                assert(m_users.count(fd)>0);
                DealWrite(&m_users[fd]);
            }
            else
            {
                LOG_ERROR("unexpectd event");
            }
        }
    }
}

//发送错误信息
void Webserver::SendError(int fd,const char* info)
{
    assert(fd>0);
    int ret=send(fd,info,strlen(info),0);
    if(ret<0)
    {
        LOG_WARN("send error to client[%d] error!",fd);
    }
    close(fd);
}

//关闭sever服务
void Webserver::CloseConn(HttpConn* client)
{
    assert(client);
    LOG_INFO("Client[%d] quit!",client->GetFd());
    m_epoller->Delfd(client->GetFd());
    client->Close();
}

//添加新的客户连接
void Webserver::AddClient(int fd,sockaddr_in addr)
{
    assert(fd>0);
    //初始化http连接
    m_users[fd].init(fd,addr);
    if(m_timeoutMS>0)
    {
        //添加新的定时
        m_timer->add(fd,m_timeoutMS,bind(&Webserver::CloseConn,this,&m_users[fd]));
    }
    //添加新的fd
    m_epoller->Addfd(fd,EPOLLIN|m_connevent);
    //设置fd为非阻塞
    SetFdNonblock(fd);
    LOG_INFO("Client[%d] in!",m_users[fd].GetFd());
}

//处理新的监听
void Webserver::DealListen()
{
    struct sockaddr_in addr;
    socklen_t len=sizeof(addr);
    do
    {
        //获得一个可以通信的套接字
        int fd=accept(m_listenfd,(struct sockaddr*)&addr,&len);
        if(fd<=0)
        {
            return;
        }
        //超过最大连接数
        else if(HttpConn::m_usercount>=MAX_FD)
        {
            SendError(fd,"Server busy!");
            LOG_WARN("Client is full");
            return;
        }
        //添加新的连接
        AddClient(fd,addr);
    }while(m_listenevent&EPOLLET);
}

//处理数据读
void Webserver::DealRead(HttpConn* client)
{
    assert(client);
    ExtendTime(client);
    //向线程池中添加读任务
    m_threadpool->AddTask(bind(&Webserver::OnRead,this,client));
}

//处理写
void Webserver::DealWrite(HttpConn* client)
{
    assert(client);
    ExtendTime(client);
    //向线程池添加写任务
    m_threadpool->AddTask(bind(&Webserver::OnWrite,this,client));
}

//调整定时器时间
void Webserver::ExtendTime(HttpConn* client)
{
    assert(client);
    if(m_timeoutMS>0)
    {
        m_timer->adjust(client->GetFd(),m_timeoutMS);
    }
}

//正在处理数据读取
void  Webserver::OnRead(HttpConn* client)
{
    assert(client);
    int ret=-1;
    int readerrno=0;
    //处理客户数据读取
    ret=client->read(&readerrno);
    if(ret<=0&&readerrno!=EAGAIN)
    {
        CloseConn(client);
        return;
    }
    //处理客户信息
    OnProcess(client);
}

void Webserver::OnProcess(HttpConn* client)
{
    //处理http请求过程
    if(client->process())
    {
        //修改epoll事件为写
        m_epoller->Modfd(client->GetFd(),m_connevent|EPOLLOUT);
    }
    else
    {
        //修改成读
        m_epoller->Modfd(client->GetFd(),m_connevent|EPOLLIN);
    }
}

//正在处理写
void Webserver::OnWrite(HttpConn* client)
{
    assert(client);
    int ret=-1;
    int writeerrno=0;
    ret=client->write(&writeerrno);
    if(client->toWriteBytes()==0)
    {
        //传输完成
        if(client->isKeepAlive())
        {
            OnProcess(client);
            return;
        }
    }
    else if(ret<0)
    {
        //继续尝试传输数据
        if(writeerrno==EAGAIN)
        {
            m_epoller->Modfd(client->GetFd(),m_connevent|EPOLLOUT);
            return;
        }
    }
    CloseConn(client);
}

//设置fd为非阻塞
int Webserver::SetFdNonblock(int fd)
{
    assert(fd>0);
    return fcntl(fd,F_SETFL,fcntl(fd,F_GETFD,0)|O_NONBLOCK);
}

//创建listenfd
bool Webserver::InitSocket()
{
    int ret;
    struct sockaddr_in addr;
    if(m_port>65535||m_port<1024)
    {
        LOG_ERROR("Port:%d error!",m_port);
        return false;
    }
    bzero(&addr,sizeof(addr));
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=htonl(INADDR_ANY);
    addr.sin_port=htons(m_port);
    struct linger optlinger={0};
    
    if(m_optlinger)
    {
        //优雅关闭连接，直到所剩的数据发送完毕或者超时
        optlinger.l_onoff=1;
        optlinger.l_linger=1;
    }
    //创建套接字
    m_listenfd=socket(AF_INET,SOCK_STREAM,0);
    if(m_listenfd<0)
    {
        LOG_ERROR("Create socket error!",m_port);
        return false;
    }
    ret=setsockopt(m_listenfd,SOL_SOCKET,SO_LINGER,&optlinger,sizeof(optlinger));
    if(ret<0)
    {
        close(m_listenfd);
        LOG_ERROR("Init linger error",m_port);
        return false;
    }
    int optval=1;
    //创建端口复用
    ret=setsockopt(m_listenfd,SOL_SOCKET,SO_REUSEADDR,(const void*)&optval,sizeof(int));
    if(ret==-1)
    {
        LOG_ERROR("set socket setsocketopt error!");
        close(m_listenfd);
        return false;
    }
    //绑定端口
    ret=bind(m_listenfd,(struct sockaddr*)&addr,sizeof(addr));
    if(ret<0)
    {
        LOG_ERROR("Bind Port:%d error!",m_port);
        close(m_listenfd);
        return false;
    }
    //监听
    ret=listen(m_listenfd,6);
    if(ret<0)
    {
        LOG_ERROR("Listen port:%d error!",m_port);
        close(m_listenfd);
        return false;
    }
    //添加epoll事件
    ret=m_epoller->Addfd(m_listenfd,m_listenevent|EPOLLIN);
    if(ret==0)
    {
        LOG_ERROR("Add listenfd errror!");
        close(m_listenfd);
        return false;
    }
    //设置为非阻塞
    SetFdNonblock(m_listenfd);
    LOG_INFO("Server port:%d",m_port);
    return true;
}
