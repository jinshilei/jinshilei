#ifndef BLOCKQUEUE_H
#define BLOCKQUEUE_H

#include<mutex>
#include<deque>
#include<condition_variable>
#include<sys/time.h>

template<class T>
class BlockDeque
{
    public:
    explicit BlockDeque(size_t MaxCapacity=1024);
    ~BlockDeque();
    void clear();
    bool empty();
    bool full();
    void close();
    size_t size();
    size_t capacity();
    T front();
    T back();
    void push_back(const T& item);
    void push_front(const T& item);
    bool pop(T& item);
    bool pop(T& item,int timeout);
    void flush();
    private:
    std::deque<T> m_deq;
    size_t m_capacity;
    std::mutex mtx;
    bool isclose;
    std::condition_variable condConsumer;
    std::condition_variable condProducer;
};

//初始化阻塞队列
template<class T>
BlockDeque<T>::BlockDeque(size_t MaxCapacity):m_capacity(MaxCapacity)
{
    assert(MaxCapacity>0);
    isclose=false;
}

template<class T>
BlockDeque<T>::~BlockDeque()
{
    close();
}
//清空阻塞队列
template<class T>
void BlockDeque<T>::clear()
{
    std::lock_guard<std::mutex> locker(mtx);
    m_deq.clear();
}

//判断阻塞队列是否为空
template<class T>
bool BlockDeque<T>::empty()
{
    std::lock_guard<std::mutex> locker(mtx);
    return m_deq.empty();
}

//判断阻塞队列满没
template<class T>
bool BlockDeque<T>::full()
{
    std::lock_guard<std::mutex> locker(mtx);
    return m_deq.size()>=m_capacity;
}

//关闭阻塞对列
template<class T>
void BlockDeque<T>::close()
{
    std::lock_guard<std::mutex> locker(mtx);
    m_deq.clear();
    isclose=true;
    condConsumer.notify_all();
    condProducer.notify_all();
}

//返回阻塞队列大小
template<class T>
size_t BlockDeque<T>::size()
{
    std::lock_guard<std::mutex> locker(mtx);
    return m_deq.size();
}

//返回阻塞队列容量
template<class T>
size_t BlockDeque<T>::capacity()
{
    std::lock_guard<std::mutex> locker(mtx);
    return m_capacity;
}

//返回阻塞队列头部内容
template<class T>
T BlockDeque<T>::front()
{
    std::lock_guard<std::mutex> locker(mtx);
    return m_deq.front();
}

//返回阻塞队列尾部内容
template<class T>
T BlockDeque<T>::back()
{
    std::lock_guard<std::mutex> locker(mtx);
    return m_deq.back();
}

//向阻塞队列尾部插入内容
template<class T>
void BlockDeque<T>::push_back(const T& item)
{
    std::unique_lock<std::mutex> locker(mtx);
    while(m_deq.size()>=m_capacity)
    {
        //阻塞producer
        condProducer.wait(locker);
    }
    m_deq.push_back(item);
    //通知consumer
    condConsumer.notify_one();
}

//向阻塞队列头部插入内容
template<class T>
void BlockDeque<T>::push_front(const T& item)
{
    std::unique_lock<std::mutex> locker(mtx);
    while(m_deq.size()>=m_capacity)
    {
        condProducer.wait(locker);
    }
    m_deq.push_front(item);
    condConsumer.notify_one();
}

//删除队列头部内容
template<class T>
bool BlockDeque<T>::pop(T& item)
{
    std::unique_lock<std::mutex> locker(mtx);
    while(m_deq.empty())
    {
        condConsumer.wait(locker);
        if(isclose)
        {
            return false;
        }
    }
    item=m_deq.front();
    m_deq.pop_front();
    //通知至少一个在等待的线程
    condProducer.notify_one();
    return true;
}

//超时处理
template <class T>
bool BlockDeque<T>::pop(T& item,int timeout)
{
    std::unique_lock<std::mutex> locker(mtx);
    while(m_deq.empty())
    {
        if(condConsumer.wait_for(locker,std::chrono::seconds(timeout))==std::cv_status::timeout)
        {
            return false;
        }
        if(isclose)
        {
            return false;
        }
    }
    item=m_deq.front();
    m_deq.pop_front();
    condProducer.notify_one();
    return true;
}

template<class T>
void BlockDeque<T>::flush()
{
    condConsumer.notify_one();
}
#endif //BLOCKQUEUE_H