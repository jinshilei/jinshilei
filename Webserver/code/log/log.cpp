#include"log.h"
using namespace std;
//初始化日志
void Log::init(int level,const char* path,const char* suffix,int maxQueueCapacity)
{
    m_closelog=false;
    m_level=level;
    //设置阻塞队列长度，异步写日志
    if(maxQueueCapacity>0)
    {
        m_isasync=true;
        if(!m_deque)
        {
            std::unique_ptr<BlockDeque<std::string>> newDeque(new BlockDeque<std::string>(maxQueueCapacity));
            m_deque=std::move(newDeque);
            //创建线程异步写日志，FlushLogThread是回调函数
            std::unique_ptr<std::thread> newThread(new thread(FlushLogThread));
            m_writethread=std::move(newThread);
        }
    }
    else
    {
        m_isasync=false;
    }
    m_linecount=0;

    time_t timer=time(nullptr);
    struct tm *sysTime=localtime(&timer);
    struct tm t=*sysTime;

    m_path=path;
    m_suffix=suffix;
    char fileName[LOG_NAME_LEN]={0};
    snprintf(fileName,LOG_NAME_LEN-1,"%s/%04d_%02d_%02d%s",m_path,t.tm_year+1900,t.tm_mon+1,t.tm_mday,m_suffix);
    m_today=t.tm_mday;

    {
        std::lock_guard<std::mutex> locker(mtx);
        //初始化缓存
        m_buff.RetriveAll();
        if(m_fp)
        {
            flush();
            fclose(m_fp);
        }
        m_fp=fopen(fileName,"a");
        if(m_fp==nullptr)
        {
            //给path添加0777权限
            mkdir(m_path,S_IRWXU|S_IRWXG|S_IRWXO);
            m_fp=fopen(fileName,"a");
        }
        assert(m_fp!=nullptr);
    }
}

//单例模式：懒汉
Log* Log::Instance()
{
    static Log p;
    return &p;
}

//调用异步写日志函数
void Log::FlushLogThread()
{
    Log::Instance()->AsyncWrite();
}

//写日志内容
void Log::write(int level,const char* format,...)
{
    struct timeval now={0,0};
    gettimeofday(&now,nullptr);
    time_t sec=now.tv_sec;
    struct tm *sysTime=localtime(&sec);
    struct tm t=*sysTime;
    //可变参数列表
    va_list valist;
    //日期不是当天或者写行数大于最大行数限制
    if(m_today!=t.tm_mday||(m_linecount && m_linecount%MAX_LINES==0))
    {
        std::unique_lock<std::mutex> locker(mtx);
        flush();
        fclose(m_fp);
        //locker.unlock();
        char newFile[LOG_NAME_LEN];
        char tail[36]={0};
        snprintf(tail,36,"%04d_%02d_%02d",t.tm_year+1900,t.tm_mon+1,t.tm_mday);
        if(m_today!=t.tm_mday)
        {
            snprintf(newFile,LOG_NAME_LEN,"%s/%s%s",m_path,tail,m_suffix);
            m_today=t.tm_mday;
            m_linecount=0;
        }
        else
        {
            snprintf(newFile,LOG_NAME_LEN,"%s/%s-%d%s",m_path,tail,(m_linecount/MAX_LINES),m_suffix);
        }
        //locker.lock();
        m_fp=fopen(newFile,"a");
        assert(m_fp!=nullptr);
    }
    
    {
        std::unique_lock<std::mutex> locker(mtx);
        m_linecount++;
        int n=snprintf(m_buff.BeginWrite(),128,"%04d-%02d-%02d %02d:%02d:%02d.%06ld",
        t.tm_year+1900,t.tm_mon+1,t.tm_mday,t.tm_hour,t.tm_min,t.tm_sec,now.tv_usec);
        m_buff.HasWritten(n);
        AppendLogLevelTitle(level);
        //获取可变参数列表第一个参数位置
        va_start(valist,format);
        int m=vsnprintf(m_buff.BeginWrite(),m_buff.WriteableBytes(),format,valist);
        //清空可变参数列表
        va_end(valist);
        m_buff.HasWritten(m);
        m_buff.Append("\n\0",2);
        //异步写日志
        if(m_isasync&&m_deque&&!m_deque->full())
        {
            m_deque->push_back(m_buff.RetriveAlltoStr());
        }
        //同步写日志
        else
        {
            fputs(m_buff.Peek(),m_fp);
        }
        //清空缓冲区
        m_buff.RetriveAll();
    }
}

//强制刷新缓冲区
void Log::flush()
{
    if(m_isasync)
    {
        m_deque->flush();
    }
    //刷新到缓冲区
    fflush(m_fp);
}

//获取日志等级
int Log::getLevel()
{
    std::lock_guard<std::mutex> locker(mtx);
    return m_level;
}

//设置日志等级
void Log::setLevel(int level)
{
    std::lock_guard<std::mutex> locker(mtx);
    m_level=level;
}

Log::Log()
{
    m_linecount=0;
    m_isasync=false;
    m_writethread=nullptr;
    m_deque=nullptr;
    m_today=0;
    m_fp=nullptr;
}

//追加日志等级
void Log::AppendLogLevelTitle(int level)
{
    switch(level)
    {
        case 0:
        m_buff.Append("[debug]: ",9);
        break;
        case 1:
        m_buff.Append("[info] : ",9);
        break;
        case 2:
        m_buff.Append("[warn] : ",9);
        break;
        case 3:
        m_buff.Append("[error]: ",9);
        break;
        default:
        m_buff.Append("[info] : ",9);
        break;
    }
}

Log::~Log()
{
    //判断线程是否可以join
    if(m_writethread && m_writethread->joinable())
    {
        while(!m_deque->empty())
        {
            m_deque->flush();
        }
        m_deque->close();
        //等待回收资源
        m_writethread->join();
    }
    if(m_fp)
    {
        std::lock_guard<std::mutex> locker(mtx);
        flush();
        fclose(m_fp);
    }
}

//异步写日志
void Log::AsyncWrite()
{
    std::string str="";
    //循环从阻塞队列里面取内容
    while(m_deque->pop(str))
    {
        std::lock_guard<std::mutex> locker(mtx);
        fputs(str.c_str(),m_fp);
    }
}