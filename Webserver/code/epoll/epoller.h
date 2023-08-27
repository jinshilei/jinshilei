#ifndef EPOLLER_H
#define EPOLLER_H

#include<sys/epoll.h>
#include<fcntl.h>
#include<unistd.h>
#include<assert.h>
#include<vector>
#include<errno.h>

class Epoller 
{
    public:
    explicit Epoller(int maxEvent=1024);
    ~Epoller();
    //注册新事件
    bool Addfd(int fd,uint32_t events);
    //修改事件
    bool Modfd(int fd,uint32_t events);
    //删除事件
    bool Delfd(int fd);
    //等待事件
    int Wait(int timeoutms=-1);
    //返回事件的文件描述符
    int GetEventfd(size_t i) const;
    //返回事件
    uint32_t GetEvents(size_t i) const;
    private:
    int m_epollfd;
    std::vector<struct epoll_event> events;
};

#endif //EPOLLER_H