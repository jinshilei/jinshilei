#ifndef LOG_H
#define LOG_H

#include<mutex>
#include<string>
#include<thread>
#include<sys/time.h>
#include<string.h>
#include<stdarg.h>
#include<assert.h>
#include<sys/stat.h>
#include"blockqueue.h"
#include"../buffer/buffer.h"

class Log 
{
    public:
    //日志初始化
    void init(int level=1,const char* path="./log",const char* suffix=".log",int maxQueueCapacity=1024);
    //单例模式
    static Log* Instance();
    //异步写日志
    static void FlushLogThread();
    //写日志
    void write(int level,const char* format,...);
    //刷新缓冲区
    void flush();
    //获取日志等级
    int getLevel();
    //设置日志等级
    void setLevel(int level);
    //日志是否打开
    bool isOpen(){return !m_closelog;}
    private:
    Log();
    //添加日志等级标题
    void AppendLogLevelTitle(int level);
    virtual ~Log();
    //异步写操作
    void AsyncWrite();
    private:
    //日志路径长度
    static const int LOG_PATH_LEN=256;
    //日志名长度
    static const int LOG_NAME_LEN=256;
    //最大日志行数
    static const int MAX_LINES=50000;
    //日志文件路径
    const char* m_path;
    //日志文件名字
    const char* m_suffix;
    //日志最大行数
    int m_maxlines;
    //日志行数记录
    int m_linecount;
    //记录当前是那一天
    int m_today;
    //关闭日志
    bool m_closelog;
    Buffer m_buff;
    //日志等级
    int m_level;
    //是否同步
    bool m_isasync;
    FILE* m_fp;
    //阻塞队列
    std::unique_ptr<BlockDeque<std::string>> m_deque;
    std::unique_ptr<std::thread> m_writethread;
    std::mutex mtx;
};

#define LOG_BASE(level,format,...)\
    do{\
        Log* log=Log::Instance();\
        if(log->isOpen()&&log->getLevel()<=level){\
            log->write(level,format,##__VA_ARGS__);\
            log->flush();\
        }\
    }while(0);

#define LOG_DEBUG(format,...) do{LOG_BASE(0,format,##__VA_ARGS__)} while(0);
#define LOG_INFO(format,...) do{LOG_BASE(1,format,##__VA_ARGS__)} while(0);
#define LOG_WARN(format,...) do{LOG_BASE(2,format,##__VA_ARGS__)} while(0);
#define LOG_ERROR(format,...) do{LOG_BASE(3,format,##__VA_ARGS__)} while(0);

#endif //LOG_H