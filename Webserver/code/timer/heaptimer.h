#ifndef HEAPTIMER_H
#define HEAPTIMER_H 

#include<unordered_map>
#include<time.h>
#include<algorithm>
#include<arpa/inet.h>
#include<functional>
#include<assert.h>
#include<chrono>
#include"../log/log.h"

using TimeoutCallBack=std::function<void()>;
using Clock=std::chrono::high_resolution_clock;
using MS=std::chrono::milliseconds;
using TimeStamp=Clock::time_point;

struct TimeNode
{
    int id;
    TimeStamp expires;//定时器生效时间
    TimeoutCallBack cb;//定时器回调函数
    bool operator<(const TimeNode& t)
    {
        return expires<t.expires;
    }
};

class HeapTimer 
{
    public:
    HeapTimer()
    {
        m_heap.reserve(64);
    }
    ~HeapTimer()
    {
        clear();
    }
    void clear();
    //调整定时器
    void adjust(int id,int newexpires);
    //添加定时器
    void add(int id,int timeout,const TimeoutCallBack& cb);
    //触发回调函数，删除节点
    void dowork(int id);
    //心搏函数
    void tick();
    //删除第一个节点
    void pop();
    //下一次心搏
    int GetNextTick();
    private:
    //删除指定节点
    void del(size_t index);
    //上滤
    void siftup(size_t i);
    //下滤
    bool siftdown(size_t index,size_t n);
    //交换节点
    void swapnode(size_t i,size_t j);
    private:
    std::vector<TimeNode> m_heap;
    std::unordered_map<int,size_t> m_ref;
};

#endif //HEAPTIMER_H