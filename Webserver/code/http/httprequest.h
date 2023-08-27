#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include<unordered_map>
#include<unordered_set>
#include<string>
#include<regex>
#include<errno.h>
#include<mysql/mysql.h>

#include"../buffer/buffer.h"
#include"../log/log.h"
#include"../pool/sqlconnpool.h"
#include"../pool/sqlconnRAII.h"


class HttpRequest 
{
    public:
    //解析状态
    enum PRASE_STATE
    {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH
    };
    HttpRequest()
    {
        init();
    }
    ~HttpRequest()=default;
    //初始化请求报文
    void init();
    //解析请求报文
    bool prase(Buffer& buff);
    //返回资源路径
    std::string path() const;
    std::string& path();
    //返回请求方法GET/POST
    std::string method() const;
    //返回HTTP版本1.1
    std::string version() const;
    //返回post报文数据
    std::string GetPost(const std::string& key) const;
    std::string GetPost(const char* key) const;
    //判断是否保持连接
    bool isKeepAlive() const;
    
    private:
    //解析请求行
    bool PraseRequestLine(const std::string& line);
    //解析请求头
    void PraseHeader(const std::string& line);
    //解析请求主体
    void PraseBody(const std::string& line);
    //解析请求数据路径
    void PrasePath();
    //解析POST请求
    void PrasePost();
    //解析url编码
    void PraseFromUrlencoded();
    //用户数据验证
    static bool UserVerify(const std::string& name,const std::string& passwd,bool isLogin);
    //解析状态
    PRASE_STATE m_state;
    //请求方法
    std::string m_method;
    std::string m_path;
    std::string m_version;
    std::string m_body;

    std::unordered_map<std::string,std::string> m_header;
    std::unordered_map<std::string,std::string> m_post;
    //默认的html文件
    static const std::unordered_set<std::string> DEFAULT_HTML;
    //登录或者注册的hmtl文件
    static const std::unordered_map<std::string,int> DEFAULT_HTML_TAG;
    //转换成16进制
    static int converthex(char ch);
};

#endif //HTTPREQUEST_H