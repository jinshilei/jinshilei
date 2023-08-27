#ifndef SQLCONNPOOL_H
#define SQLCONNPOOL_H

#include<mutex>
#include<mysql/mysql.h>
#include<string>
#include<queue>
#include<semaphore.h>
#include<thread>
#include"../log/log.h"

class SqlConnPool 
{
    public:
    static SqlConnPool* Instance();
    //获取一个空闲连接
    MYSQL* GetConn();
    //释放连接
    void FreeConn(MYSQL* conn);
    //获取空闲数量
    int GetFreeConnCount();
    //初始化
    void init(const char* host,const char* user,const char* pwd,const char* dbname,int port,int connsize=8);
    //关闭数据库池
    void ClosePool();
    private:
    SqlConnPool();
    ~SqlConnPool();
    int m_maxconn;
    int m_usecount;
    int m_freecount;
    std::queue<MYSQL*> m_connqueue;
    std::mutex m_mtx;
    sem_t m_semid;
};

#endif //SQLCONNPOOL_H