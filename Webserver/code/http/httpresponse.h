#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include<unordered_map>
#include<fcntl.h>
#include<unistd.h>
#include<sys/stat.h>
#include<sys/mman.h>

#include"../buffer/buffer.h"
#include"../log/log.h"

class HttpResponse 
{
    public:
    HttpResponse();
    ~HttpResponse();
    //响应报文初始化
    void init(const std::string& srcDir,const std::string& path,bool isKeepAlive,int code=-1);
    //做出响应
    void MakeResponse(Buffer& buff);
    //取消映射
    void UnmapFile();
    char* File();
    size_t FileLen() const;
    //错误文本
    void ErrorContent(Buffer& buff,std::string message);
    int Code() const
    {
        return m_code;
    }
    private:
    //添加状态行
    void AddStateLine(Buffer& buff);
    //添加消息报头
    void AddHeader(Buffer& buff);
    //添加响应正文
    void AddContent(Buffer& buff);
    //错误的html
    void ErrorHtml();
    //获取文件类型
    std::string GetFileType();
    //状态码
    int m_code;
    //连接管理
    bool m_isKeepAlive;
    //具体的响应资源
    std::string m_path;
    //资源文件路径
    std::string m_srcDir;
    //映射文件
    char* m_mmfile;
    //文件属性
    struct stat mmFileStat;
    //存放文件类型后缀
    static const std::unordered_map<std::string,std::string> SUFFIX_TYPE;
    //存放状态码对应的错误信息
    static const std::unordered_map<int,std::string> CODE_STATUS;
    //存放错误码对应的html响应文件
    static const std::unordered_map<int,std::string> CODE_PATH;
};

#endif //HTTP_RESPONSE_H