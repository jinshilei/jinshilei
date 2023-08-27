#include"sqlconnpool.h"

//单例模式
SqlConnPool* SqlConnPool::Instance()
{
    static SqlConnPool p;
    return &p;
}
//获取一个连接
MYSQL* SqlConnPool::GetConn()
{
    MYSQL* mysql=nullptr;
    if(m_connqueue.empty())
    {
        LOG_WARN("SqlConnPool: mysql connection pool is busy now!");
        return nullptr;
    }
    //阻塞线程
    sem_wait(&m_semid);
    {
        std::lock_guard<std::mutex> locker(m_mtx);
        mysql=m_connqueue.front();
        m_connqueue.pop();
    }
    return mysql;
}

//释放连接
void SqlConnPool::FreeConn(MYSQL* conn)
{
    assert(conn);
    std::lock_guard<std::mutex> locker(m_mtx);
    m_connqueue.push(conn);
    //解除阻塞
    sem_post(&m_semid);
}

//获取空闲数量
int SqlConnPool::GetFreeConnCount()
{
    std::lock_guard<std::mutex> locker(m_mtx);
    return m_connqueue.size();
}

//初始化
void SqlConnPool::init(const char* host,const char* user,const char* pwd,const char* dbname,int port,int connsize)
{
    assert(connsize>0);
    for(int i=0;i<connsize;i++)
    {
        MYSQL *sql=nullptr;
        sql=mysql_init(sql);
        if(!sql)
        {
            LOG_ERROR("mysql init error!");
            assert(sql);
        }
        sql=mysql_real_connect(sql,host,user,pwd,dbname,port,nullptr,0);
        if(!sql)
        {
            LOG_ERROR("mysql connect error!");
        }
        m_connqueue.push(sql);
    }
    m_maxconn=connsize;
    LOG_INFO("sql pool init")
    sem_init(&m_semid,0,m_maxconn);
}

//关闭数据库池
void SqlConnPool::ClosePool()
{
    std::lock_guard<std::mutex> locker(m_mtx);
    while(!m_connqueue.empty())
    {
        auto item=m_connqueue.front();
        m_connqueue.pop();
        mysql_close(item);
    }
}

SqlConnPool::SqlConnPool()
{
    m_usecount=0;
    m_freecount=0;
}

SqlConnPool::~SqlConnPool()
{
    ClosePool();
}