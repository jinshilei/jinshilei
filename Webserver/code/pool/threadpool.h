#ifndef THREADPOOL_H
#define THREADPOOL_H

#include<mutex>
#include<condition_variable>
#include<queue>
#include<thread>
#include<functional>

class ThreadPool
{
    public:
    explicit ThreadPool(size_t threadcount=8):m_pool(std::make_shared<Pool>())
    {
        assert(threadcount>0);
        for(size_t i=0;i<threadcount;i++)
        {
            std::thread([pool=m_pool]{
                //构造时候加锁
                std::unique_lock<std::mutex> locker(pool->mtx);
                while(true)
                {
                    if(!pool->tasks.empty())
                    {
                        //取一个task
                        auto task=std::move(pool->tasks.front());
                        pool->tasks.pop();
                        //解锁
                        locker.unlock();
                        task();
                        //加锁，unique_lock析构自动解锁
                        locker.lock();
                    }
                    //线程池停止使用，循环停止
                    else if(pool->isclosed)
                    {
                        break;
                    }
                    //任务队列为空，线程等待
                    else
                    {
                        pool->cond.wait(locker);
                    }
                }
            }).detach();//线程分离
        }
    }
    ThreadPool()=default;
    //移动构造
    ThreadPool(ThreadPool&&)=default;
    ~ThreadPool()
    {
        if(static_cast<bool>(m_pool))
        {
            std::lock_guard<std::mutex> locker(m_pool->mtx);
            m_pool->isclosed=true;
            //唤醒所有阻塞线程
            m_pool->cond.notify_all();
        }
    }
    //向线程池添加一个任务
    template<class F>
    void AddTask(F&& task)
    {
        std::lock_guard<std::mutex> locker(m_pool->mtx);
        m_pool->tasks.emplace(std::forward<F>(task));
        //唤醒一个线程
        m_pool->cond.notify_one();
    }
    private:
    struct Pool
    {
        //锁
        std::mutex mtx;
        //条件阻塞
        std::condition_variable cond;
        //任务队列
        std::queue<std::function<void()>> tasks;
        //是否执行
        bool isclosed;
    };
    std::shared_ptr<Pool> m_pool;
};

#endif //THRADPOOL_H