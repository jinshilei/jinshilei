#include"httpconn.h"

using namespace std;

bool HttpConn::m_isET;
const char* HttpConn::m_srcDir;
std::atomic<int> HttpConn::m_usercount;

HttpConn::HttpConn()
{
    m_fd=-1;
    m_addr={0};
    m_isclose=true;
}

HttpConn::~HttpConn()
{
    Close();
}

//初始化http连接
void HttpConn::init(int fd,const sockaddr_in& addr)
{
    assert(fd>0);
    m_usercount++;
    m_addr=addr;
    m_fd=fd;
    m_writebuff.RetriveAll();
    m_readbuff.RetriveAll();
    m_isclose=false;
    LOG_INFO("Client[%d](%s:%d) on line,usercount:%d",m_fd,GetIP(),GetPort(),(int)m_usercount);
}

//关闭http连接
void HttpConn::Close()
{
    m_response.UnmapFile();
    if(m_isclose==false)
    {
        m_isclose=true;
        m_usercount--;
        close(m_fd);
        LOG_INFO("Client[%d](%s:%d) quit,usercount:%d",m_fd,GetIP(),GetPort(),(int)m_usercount);
    }
}

int HttpConn::GetFd() const
{
    return m_fd;
}

struct sockaddr_in HttpConn::GetAddr() const 
{
    return m_addr;
}

//获取IP地址
const char* HttpConn::GetIP() const 
{
    return inet_ntoa(m_addr.sin_addr);
}

//获取端口号
int HttpConn::GetPort() const
{
    return m_addr.sin_port;
}

//将m_fd中的信息读取到m_readbuff中
ssize_t HttpConn::read(int* saveErrno)
{
    ssize_t len=-1;
    do
    {
        //读取http的请求连接信息
        len=m_readbuff.ReadFd(m_fd,saveErrno);
        if(len<=0)
        {
            break;
        }
    }while(m_isET);
    return len;
}

//向m_fd中写数据,writev聚集写
ssize_t HttpConn::write(int* saveErrno)
{
    ssize_t len=-1;
    do
    {
        //向fd中写数据
        len=writev(m_fd,m_iov,m_iovcnt);
        if(len<=0)
        {
            *saveErrno=errno;
            break;
        }
        //数据传送完
        if(m_iov[0].iov_len+m_iov[1].iov_len==0)
        {
            break;
        }
        //第一块分散的缓冲区写完
        else if(static_cast<size_t>(len)>m_iov[0].iov_len)
        {
            //继续向第二块缓冲区写数据
            m_iov[1].iov_base=(uint8_t*)m_iov[1].iov_base+(len-m_iov[0].iov_len);
            m_iov[1].iov_len=m_iov[1].iov_len-(len-m_iov[0].iov_len);
            //清空第一块缓冲区数据
            if(m_iov[0].iov_len)
            {
                m_writebuff.RetriveAll();
                m_iov[0].iov_len=0;
            }
        }
        else
        {
            m_iov[0].iov_base=(uint8_t*)m_iov[0].iov_base+len;
            m_iov[0].iov_len=m_iov[0].iov_len-len;
            //移动写指针
            m_writebuff.Retrive(len);
        }
    }while(m_isET||toWriteBytes()>10240);//要写数据很大
    return len;
}

//处理http连接过程
bool HttpConn::process()
{
    //初始化请求
    m_request.init();
    //判断是否可读
    if(m_readbuff.ReadableBytes()<=0)
    {
        return false;
    }
    //解析http请求
    else if(m_request.prase(m_readbuff))
    {
        LOG_DEBUG("request path:%s",m_request.path().c_str());
        //初始化http响应报文
        m_response.init(m_srcDir,m_request.path(),m_request.isKeepAlive(),200);
    }
    else
    {
        m_response.init(m_srcDir,m_request.path(),false,400);
    }
    //向m_writebuff里面写响应报文
    m_response.MakeResponse(m_writebuff);

    m_iov[0].iov_base=const_cast<char*>(m_writebuff.Peek());
    m_iov[0].iov_len=m_writebuff.ReadableBytes();
    m_iovcnt=1;
    //响应正文有内容
    if(m_response.FileLen()>0&&m_response.File())
    {
        m_iov[1].iov_base=m_response.File();
        m_iov[1].iov_len=m_response.FileLen();
        m_iovcnt=2;
    }
    LOG_DEBUG("filesize:%d,%d to %d",m_response.FileLen(),m_iovcnt,toWriteBytes());
    return true;
}