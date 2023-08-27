#include"httprequest.h"

using namespace std;

const unordered_set<string> HttpRequest::DEFAULT_HTML{
    "/index","/register","/login","/welcome","/video","/picture"};

const unordered_map<string,int> HttpRequest::DEFAULT_HTML_TAG{
    {"/register.html",0},{"/login.html",1}
};

void HttpRequest::init()
{
    m_path=m_method=m_body=m_version="";
    m_state=REQUEST_LINE;
    m_header.clear();
    m_post.clear();
}

//是否保持连接
bool HttpRequest::isKeepAlive() const
{
    if(m_header.count("Connection")==1)
    {
        return m_header.find("Connection")->second=="keep-alive"&&m_version=="1.1";
    }
    return false;
}

//解析请求url
bool HttpRequest::prase(Buffer& buff)
{
    const char CRLF[]="\r\n";
    if(buff.ReadableBytes()<=0)
    {
        return false;
    }
    //缓冲区有数据可读和解析状态不结束
    while(buff.ReadableBytes()&&m_state!=FINISH)
    {
        //查找buff中\r\n第一次出现的位置
        const char* lineEnd=search(buff.Peek(),buff.BeginWriteConst(),CRLF,CRLF+2);
        //需要解析的一行数据
        string line(buff.Peek(),lineEnd);
        switch(m_state)
        {
            case REQUEST_LINE:
            if(!PraseRequestLine(line))
            {
                return false;
            }
            PrasePath();
            break;
            case HEADERS:
            PraseHeader(line);
            if(buff.ReadableBytes()<=2)
            {
                m_state=FINISH;
            }
            break;
            case BODY:
            PraseBody(line);
            break;
            default:
            break;
        }
        if(lineEnd==buff.BeginWrite())
        {
            break;
        }
        //移动读指针位置，移动到下一行开始位置
        buff.RetriveUntil(lineEnd+2);
    }
    LOG_DEBUG("[%s],[%s],[%s]",m_method.c_str(),m_path.c_str(),m_version.c_str());
    return true;
}

//解析请求文件位置
void HttpRequest::PrasePath()
{
    if(m_path=="/")
    {
        m_path="/index.html";
    }
    else
    {
        for(auto &item:DEFAULT_HTML)
        {
            if(item==m_path)
            {
                m_path+=".html";
                break;
            }
        }
    }
}

//用正则表达式解析请求行
bool HttpRequest::PraseRequestLine(const string& line)
{
    //匹配方法 路径 版本号
    regex pattern("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    smatch submatch;
    if(regex_match(line,submatch,pattern))
    {
        m_method=submatch[1];
        m_path=submatch[2];
        m_version=submatch[3];
        m_state=HEADERS;
        return true;
    }
    LOG_ERROR("request line parase error");
    return false;
}

//用正则表达式解析请求头部
void HttpRequest::PraseHeader(const string& line)
{
    regex pattern("^([^:]*): ?(.*)$");
    smatch submatch;
    if(regex_match(line,submatch,pattern))
    {
        m_header[submatch[1]]=submatch[2];
    }
    else
    {
        m_state=BODY;
    }
}

//解析请求数据
void HttpRequest::PraseBody(const string& line)
{
    m_body=line;
    PrasePost();
    m_state=FINISH;
    LOG_DEBUG("Body:%s,len:%d",line.c_str(),line.size());
}

//解析POST请求
void HttpRequest::PrasePost()
{
    if(m_method=="POST"&&m_header["Content-Type"]=="application/x-www-form-urlencoded")
    {
        PraseFromUrlencoded();
        if(DEFAULT_HTML_TAG.count(m_path))
        {
            int tag=DEFAULT_HTML_TAG.find(m_path)->second;
            LOG_DEBUG("Tag:%d",tag);
            if(tag==0||tag==1)
            {
                //false是注册，true是登录
                bool islogin=(tag==1);
                if(UserVerify(m_post["username"],m_post["password"],islogin))
                {
                    m_path="/welcome.html";
                }
                else
                {
                    m_path="/error.html";
                }
            }
        }
    }
}

//转换成16进制
int HttpRequest::converthex(char ch)
{
    if(ch>='A'&&ch<='F')
    {
        return ch-'A'+10;
    }
    if(ch>='a'&&ch<='f')
    {
        return ch-'a'+10;
    }
    return ch;
}

//解析URL的编码规则
void HttpRequest::PraseFromUrlencoded()
{
    if(m_body.size()==0)
    {
        return;
    }
    string key,value;
    int num=0;
    int n=m_body.size();
    int i=0,j=0;

    for(;i<n;i++)
    {
        char ch=m_body[i];
        switch(ch)
        {
            case '=':
            {
                key=m_body.substr(j,i-j);
                j=i+1;
            }
            break;
            case '+':
            {
                m_body[i]=' ';
            }
            break;
            case '%':
            {
                //进制转换
                num=converthex(m_body[i+1])*16+converthex(m_body[i+2]);
                m_body[i+1]=num/10+'0';
                m_body[i+2]=num%10+'0';
                i+=2;
            }
            break;
            case '&':
            {
                value=m_body.substr(j,i-j);
                j=i+1;
                m_post[key]=value;
                LOG_DEBUG("%s=%s",key.c_str(),value.c_str());
            }
            break;
            default:
            break;
        }
    }
    assert(j<=i);
    if(m_post.count(key)==0&&j<i)
    {
        value=m_body.substr(j,i-j);
        m_post[key]=value;
    }
}

//用户验证
bool HttpRequest::UserVerify(const string& name,const string& passwd,bool isLogin)
{
    if(name==""||passwd=="")
    {
        return false;
    }
    LOG_INFO("verify name:%s passwd:%s",name.c_str(),passwd.c_str());
    MYSQL *sql=nullptr;
    auto connpool= SqlConnPool::Instance();
    //创建数据库连接池
    SqlConnRAII tempval(&sql,connpool);
    assert(sql);
    bool flag=false;
    MYSQL_RES *res=nullptr;
    char order[256]={0};
    if(!isLogin)
    {
        flag=true;
    }
    snprintf(order,256,"select username,passwd from user where username='%s' limit 1",name.c_str());
    LOG_DEBUG("order:%s",order);
    //sql语句执行失败返回
    if(mysql_query(sql,order))
    {
        mysql_free_result(res);
        LOG_ERROR("sql query error");
        return false;
    }
    //获取结果集
    res=mysql_store_result(sql);

    //判断是登录或者注册
    while(MYSQL_ROW row=mysql_fetch_row(res))
    {
        LOG_DEBUG("MYSQL ROW:%s %s",row[0],row[1]);
        string password(row[1]);
        //如果是登录
        if(isLogin)
        {
            //密码正确，flag为true
            if(passwd==password)
            {
                flag=true;
            }
            else
            {
                flag=false;
                LOG_DEBUG("password error!");
            }
        }
        //是注册，用户名被使用
        else
        {
            flag=false;
            LOG_DEBUG("username used!");
        }
    }
    //释放结果集
    mysql_free_result(res);
    //注册，用户名没被使用
    if(!isLogin&&flag==true)
    {
        LOG_DEBUG("register user!");
        bzero(order,256);
        snprintf(order,256,"insert into user(username,passwd) values('%s','%s')",name.c_str(),passwd.c_str());
        LOG_DEBUG("%s",order);
        if(mysql_query(sql,order))
        {
            LOG_DEBUG("insert users error");
            flag=false;
        }
        else
        {
            flag=true;
        }
    }
    //释放数据库连接
    SqlConnPool::Instance()->FreeConn(sql);
    LOG_DEBUG("user verify successfull!");
    return flag;
}

string HttpRequest::path() const
{
    return m_path;
}
string& HttpRequest::path()
{
    return m_path;
}
string HttpRequest::method() const
{
    return m_method;
}
string HttpRequest::version() const
{
    return m_version;
}
string HttpRequest::GetPost(const string& key) const
{
    assert(key!="");
    if(m_post.count(key)==1)
    {
        return m_post.find(key)->second;
    }
    return "";
}
string HttpRequest::GetPost(const char* key) const
{
    assert(key==nullptr);
    if(m_post.count(key)==1)
    {
        return m_post.find(key)->second;
    }
    return "";
}