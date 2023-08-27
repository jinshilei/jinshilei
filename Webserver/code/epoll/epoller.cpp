#include"epoller.h"

Epoller::Epoller(int maxEvent):m_epollfd(epoll_create(512)),events(maxEvent)
{
    assert(m_epollfd>=0&&events.size()>0);
}

Epoller::~Epoller()
{
    close(m_epollfd);
}

//注册事件
bool Epoller::Addfd(int fd,uint32_t events)
{
    if(fd<0)
    {
        return false;
    }
    epoll_event ev={0};
    ev.data.fd=fd;
    ev.events=events;
    return 0==epoll_ctl(m_epollfd,EPOLL_CTL_ADD,fd,&ev);
}

//修改事件
bool Epoller::Modfd(int fd,uint32_t events)
{
    if(fd<0)
    {
        return false;
    }
    epoll_event ev={0};
    ev.data.fd=fd;
    ev.events=events;
    return 0==epoll_ctl(m_epollfd,EPOLL_CTL_MOD,fd,&ev);
}

//删除事件
bool Epoller::Delfd(int fd)
{
    if(fd<0)
    {
        return false;
    }
    epoll_event ev={0};
    return 0==epoll_ctl(m_epollfd,EPOLL_CTL_DEL,fd,&ev);
}

//等待epoll文件描述符上IO事件
int Epoller::Wait(int timeoutms)
{
    return epoll_wait(m_epollfd,&events[0],static_cast<int>(events.size()),timeoutms);
}

//返回文件描述符
int Epoller::GetEventfd(size_t i) const
{
    assert(i<events.size()&&i>=0);
    return events[i].data.fd;
}

//返回epoll事件
uint32_t Epoller::GetEvents(size_t i) const
{
    assert(i<events.size()&&i>=0);
    return events[i].events;
}