#ifndef BUFFER_H
#define BUFFER_H
#include<cstring>
#include<iostream>
#include<unistd.h>
#include<sys/uio.h>
#include<vector>
#include<atomic>
#include<assert.h>

class Buffer{
    public:
    Buffer(int initBuffSize=1024);
    ~Buffer()=default;
    //剩余可写字节
    size_t WriteableBytes() const;
    //可读的字节数
    size_t ReadableBytes() const;
    //读指针位置
    size_t PrependableBytes() const;
    //开始读的字符位置
    const char* Peek() const;
    //确保缓冲区足够空间可写
    void EnsureWriteable(size_t len);
    //移动写指针位置
    void HasWritten(size_t len);
    //移动读指针位置
    void Retrive(size_t len);
    void RetriveUntil(const char* end);
    void RetriveAll();
    //可读数据转换成字符串
    std::string RetriveAlltoStr();
    //开始写位置
    const char* BeginWriteConst() const;
    char* BeginWrite();
    //追加数据
    void Append(const std::string& str);
    void Append(const char* str,size_t len);
    void Append(const void * data,size_t len);
    void Append(const Buffer& buff);
    //读fd数据
    ssize_t ReadFd(int fd,int *Errno);
    //写fd数据
    ssize_t WriteFd(int fd,int *Errno);
    private:
    char* BeginPtr();
    const char*BeginPtr() const;
    //扩充缓冲区长度
    void MakeSpace(size_t len);

    std::vector<char> m_buffer;
    //确保同一时刻只有一个线程访问资源
    std::atomic<std::size_t> m_readpos;
    std::atomic<std::size_t> m_writepos;
};


#endif 