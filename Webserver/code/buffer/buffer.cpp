#include"buffer.h"
Buffer::Buffer(int initBuffSize):m_buffer(initBuffSize),m_readpos(0),m_writepos(0){}

//返回可写字节数
size_t Buffer::WriteableBytes() const
{
   return m_buffer.size()-m_writepos;
}

//返回可读字节数
size_t Buffer::ReadableBytes() const
{
    return m_writepos-m_readpos;
}

//返回读指针位置
size_t Buffer::PrependableBytes() const
{
    return m_readpos;
}

//返回可读的字符
const char* Buffer::Peek() const
{
    return BeginPtr()+m_readpos;
}

//确保足够空间写
void Buffer::EnsureWriteable(size_t len)
{
    if(WriteableBytes()<len)
    {
        MakeSpace(len);
    }
    assert(WriteableBytes()>=len);
}

//移动写指针位置
void Buffer::HasWritten(size_t len)
{
    m_writepos+=len;
}

//读指针偏移len位置
void Buffer::Retrive(size_t len)
{
    assert(len<=ReadableBytes());
    m_readpos+=len;
}

//读指针偏移
void Buffer::RetriveUntil(const char* end)
{
    assert(Peek()<=end);
    Retrive(end-Peek());
}

//清空缓冲区数据
void Buffer::RetriveAll()
{
    bzero(&m_buffer[0],m_buffer.size());
    m_readpos=0;
    m_writepos=0;
}

//读取数据转换成字符串
std::string Buffer::RetriveAlltoStr()
{
    std::string str(Peek(),ReadableBytes());
    RetriveAll();
    return str;
}

//开始写的位置
const char* Buffer::BeginWriteConst() const
{
    return BeginPtr()+m_writepos;
}

//开始写位置
char* Buffer::BeginWrite()
{
    return BeginPtr()+m_writepos;
}

//追加数据
void Buffer::Append(const std::string& str)
{
    Append(str.data(),str.size());
}

//追加长度len的数据
void Buffer::Append(const char* str,size_t len)
{
    assert(str);
    EnsureWriteable(len);
    std::copy(str,str+len,BeginWrite());
    HasWritten(len);
}

//追加长度len的数据
void Buffer::Append(const void * data,size_t len)
{
    assert(data);
    Append(static_cast<const char*>(data),len);
}

//追加数据
void Buffer::Append(const Buffer& buff)
{
    Append(buff.Peek(),buff.ReadableBytes());
}

//向fd中读取数据
ssize_t Buffer::ReadFd(int fd,int *Errno)
{
    char buff[65535];
    //分散读，保证一次性读完所有数据
    struct iovec iov[2];
    const size_t writeable=WriteableBytes();
    iov[0].iov_base=BeginPtr()+m_writepos;
    iov[0].iov_len=writeable;
    iov[1].iov_base=buff;
    iov[1].iov_len=sizeof(buff);
    //返回读的总字节数
    const ssize_t len=readv(fd,iov,2);
    if(len<0)
    {
        *Errno=errno;
    }
    else if(static_cast<size_t>(len)<=writeable)
    {
        m_writepos+=len;
    }
    else
    {
        m_writepos=m_buffer.size();
        Append(buff,len-writeable);
    }
    return len;
}

//向fd中写数据
ssize_t Buffer::WriteFd(int fd,int *Errno)
{
    size_t readsize=ReadableBytes();
    ssize_t len=write(fd,Peek(),readsize);
    if(len<0)
    {
        *Errno=errno;
        return len;
    }
    m_readpos+=len;
    return len;
}

//返回字符串开始位置
char* Buffer::BeginPtr()
{
    return &*m_buffer.begin();
}

const char*Buffer::BeginPtr() const
{
    return &*m_buffer.begin();
}


//扩容len长
void Buffer::MakeSpace(size_t len)
{
    //当剩余空间不够写，可写空间+已经读空间<新增长度
    if(WriteableBytes()+PrependableBytes()<len)
    {
        m_buffer.resize(len+1+m_writepos);
    }
    //空间足够写
    else
    {
        size_t readble=ReadableBytes();
        //将读指针和写指针之间的数据拷贝到缓冲区最前面
        std::copy(BeginPtr()+m_readpos,BeginPtr()+m_writepos,BeginPtr());
        m_readpos=0;
        //移动写指针
        m_writepos=m_readpos+readble;
        assert(readble==ReadableBytes());
    }
}