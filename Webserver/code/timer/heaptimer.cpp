#include"heaptimer.h"

//上滤
void HeapTimer::siftup(size_t i)
{
    assert(i>=0&&i<m_heap.size());
    size_t j=(i-1)/2;
    while(j>=0)
    {
        if(m_heap[j]<m_heap[i])
        {
            break;
        }
        swapnode(i,j);
        i=j;
        j=(i-1)/2;
    }
}

void HeapTimer::swapnode(size_t i,size_t j)
{
    assert(i>=0&&i<m_heap.size());
    assert(j>=0&&j<m_heap.size());
    std::swap(m_heap[i],m_heap[j]);
    m_ref[m_heap[i].id]=i;
    m_ref[m_heap[j].id]=j;
}

//下滤
bool HeapTimer::siftdown(size_t index,size_t n)
{
    assert(index>=0&&index<m_heap.size());
    assert(n>=0&&n<=m_heap.size());
    size_t i=index;
    size_t j=i*2+1;
    while(j<n)
    {
        if(j+1<n&&m_heap[j+1]<m_heap[j])
        {
            j++;
        }
        if(m_heap[i]<m_heap[j])
        {
            break;
        }
        i=j;
        j=i*2+1;
    }
    return i>index;
}

//添加定时器
void HeapTimer::add(int id,int timeout,const TimeoutCallBack& cb)
{
    assert(id>=0);
    size_t i;
    //新节点，堆尾插入
    if(m_ref.count(id)==0)
    {
        i=m_heap.size();
        m_ref[id]=i;
        m_heap.push_back({id,Clock::now()+MS(timeout),cb});
        //调整堆进行上滤操作
        siftup(i);
    }
    else
    {
        i=m_ref[id];
        m_heap[i].expires=Clock::now()+MS(timeout);
        m_heap[i].cb=cb;
        //调整堆
        if(!siftdown(i,m_heap.size()))
        {
            siftup(i);
        }
    }
}

void HeapTimer::dowork(int id)
{
    if(m_heap.empty()||m_ref.count(id)==0)
    {
        return;
    }
    size_t i=m_ref[id];
    TimeNode node=m_heap[i];
    //触发回调函数
    node.cb();
    //删除i节点
    del(i);
}

void HeapTimer::del(size_t index)
{
    assert(!m_heap.empty()&&index>=0&&index<m_heap.size());
    size_t i=index;
    size_t n=m_heap.size()-1;
    assert(i<=n);
    //调整删除元素到队尾
    if(i<n)
    {
        swapnode(i,n);
        if(!siftdown(i,n))
        {
            siftup(i);
        }
    }
    //删除队尾元素
    m_ref.erase(m_heap.back().id);
    m_heap.pop_back();
}

//调整节点
void HeapTimer::adjust(int id,int expires)
{
    assert(!m_heap.empty()&&m_ref.count(id)>0);
    m_heap[m_ref[id]].expires=Clock::now()+MS(expires);
    siftdown(m_ref[id],m_heap.size());
}

//删除超时节点
void HeapTimer::tick()
{
    if(m_heap.empty())
    {
        return;
    }
    //循环处理到期定时器
    while(!m_heap.empty())
    {
        TimeNode node=m_heap.front();
        if(std::chrono::duration_cast<MS>(node.expires-Clock::now()).count()>0)
        {
            break;
        }
        node.cb();
        pop();
    }
}

//删除第一个节点
void HeapTimer::pop()
{
    assert(!m_heap.empty());
    del(0);
}

//清空堆
void HeapTimer::clear()
{
    m_ref.clear();
    m_heap.clear();
}

//下一次心搏
int HeapTimer::GetNextTick()
{
    tick();
    size_t res=-1;
    if(!m_heap.empty())
    {
        res=std::chrono::duration_cast<MS>(m_heap.front().expires-Clock::now()).count();
        if(res<0)
        {
            res=0;
        }
    }
    return res;
}