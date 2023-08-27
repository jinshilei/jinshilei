#include"httpresponse.h"
using namespace std;

//文件后缀名
const unordered_map<string,string> HttpResponse::SUFFIX_TYPE={
    {".html","text/html"},
    {".xml","text/xml"},
    {".xhtml","application/xhtml+xml"},
    {".txt","text/plain"},
    {".rtf","application/rtf"},
    {".word","application/nsword"},
    {".png","image/png"},
    {".gif","image/gif"},
    {".jpeg","image/jpeg"},
    {".jpg","image/jpg"},
    {".au","audio/basic"},
    {".mpeg","video/mpeg"},
    {".mpg","video/mpeg"},
    {".avi","video/mpeg"},
    {".gz","application/x-gzip"},
    {".tar","application/x-tar"},
    {".css","text/css"},
    {".js","text/javascript"}
};

//状态码
const unordered_map<int,string> HttpResponse::CODE_STATUS{
    {200,"OK"},
    {400,"Bad Request"},
    {403,"Forbidden"},
    {404,"Not Found"}
};

//错误文件信息存放路径
const unordered_map<int,string> HttpResponse::CODE_PATH={
    {400,"/400.html"},
    {403,"/403.html"},
    {404,"/404.html"}
};

HttpResponse::HttpResponse()
{
    m_code=-1;
    m_path="";
    m_srcDir="";
    m_isKeepAlive=false;
    mmFileStat={0};
}

HttpResponse::~HttpResponse()
{
    UnmapFile();
}

void HttpResponse::init(const string& srcDir,const string& path,bool isKeepAlive,int code)
{
    assert(srcDir!="");
    if(m_mmfile)
    {
        UnmapFile();

    }
    m_code=code;
    m_isKeepAlive=isKeepAlive;
    m_path=path;
    m_srcDir=srcDir;
    m_mmfile=nullptr;
    mmFileStat={0};
}

//响应http请求
void HttpResponse::MakeResponse(Buffer& buff)
{
    //请求资源找不到
    if(stat((m_srcDir+m_path).data(),&mmFileStat)<0||S_ISDIR(mmFileStat.st_mode))
    {
        m_code=404;
    }
    //禁止访问资源
    else if(!(mmFileStat.st_mode&S_IROTH))
    {
        m_code=403;
    }
    else if(m_code==-1)
    {
        m_code=200;
    }
    ErrorHtml();
    AddStateLine(buff);
    AddHeader(buff);
    AddContent(buff);
}

char* HttpResponse::File()
{
    return m_mmfile;
}

size_t HttpResponse::FileLen() const
{
    return mmFileStat.st_size;
}

//错误码对应的html文件
void HttpResponse::ErrorHtml()
{
    if(CODE_PATH.count(m_code)==1)
    {
        m_path=CODE_PATH.find(m_code)->second;
        //获取文件信息到mmFileStat里
        stat((m_srcDir+m_path).data(),&mmFileStat);
    }
}

//添加状态行
void HttpResponse::AddStateLine(Buffer& buff)
{
    string status;
    if(CODE_STATUS.count(m_code)==1)
    {
        status=CODE_STATUS.find(m_code)->second;
    }
    else
    {
        m_code=400;
        status=CODE_STATUS.find(m_code)->second;
    }
    buff.Append("HTTP/1.1 "+to_string(m_code)+" "+status+"\r\n");
}

//添加消息报头
void HttpResponse::AddHeader(Buffer& buff)
{
    buff.Append("Connection: ");
    if(m_isKeepAlive)
    {
        buff.Append("keep-alive\r\n");
        buff.Append("keep-alive: max=6,timeout=120\r\n");
    }
    else
    {
        buff.Append("close\r\n");
    }
    buff.Append("Content-type: "+GetFileType()+"\r\n");
}

//添加响应正文
void HttpResponse::AddContent(Buffer& buff)
{
    int srcfd=open((m_srcDir+m_path).data(),O_RDONLY);
    if(srcfd<0)
    {
        ErrorContent(buff,"File Not Found!");
        return;
    }
    LOG_DEBUG("file path:%s",(m_srcDir+m_path).data());
    //文件映射到内存提高文件访问速度
    int* mmfile=(int*)mmap(0,mmFileStat.st_size,PROT_READ,MAP_PRIVATE,srcfd,0);
    if(*mmfile==-1)
    {
        ErrorContent(buff,"file not found!");
        return;
    }
    m_mmfile=(char*)mmfile;
    close(srcfd);
    buff.Append("Content-length: "+to_string(mmFileStat.st_size)+"\r\n\r\n");
}

//取消文件映射
void HttpResponse::UnmapFile()
{
    if(m_mmfile)
    {
        munmap(m_mmfile,mmFileStat.st_size);
        m_mmfile=nullptr;
    }
}

string HttpResponse::GetFileType()
{
    //返回.最后一次出现位置
    string::size_type idx=m_path.find_last_of('.');
    if(idx==string::npos)
    {
        return "text/plain";
    }
    string suffix=m_path.substr(idx);
    if(SUFFIX_TYPE.count(suffix)==1)
    {
        return SUFFIX_TYPE.find(suffix)->second;
    }
    return "text/plain";
}

//错误的响应正文
void HttpResponse::ErrorContent(Buffer& buff,string message)
{
    string body;
    string status;
    body+="<html><title>Error</title>";
    body+="<body bgcolor=\"ffffff\">";
    if(CODE_STATUS.count(m_code)==1)
    {
        status=CODE_STATUS.find(m_code)->second;
    }
    else
    {
        status="Bad Request";
    }
    body+=to_string(m_code)+" : "+status+"\n";
    body+="<p>"+message+"</p>";
    body+="<hr><em>TinyWebserver</em></body></html>";
    buff.Append("Content-length: "+to_string(body.size())+"\r\n\r\n");
    buff.Append(body);
}