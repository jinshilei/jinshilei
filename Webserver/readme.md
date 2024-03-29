# WebServer
用C++实现的WEB服务器，经过webbenchh压力测试有上万并发数

## 功能
* 利用IO复用技术Epoll与线程池实现多线程的Reactor高并发模型；
* 利用正则与状态机解析HTTP请求报文，实现处理静态资源的请求；
* 利用标准库容器封装char，实现自动增长的缓冲区；
* 基于小根堆实现的定时器，关闭超时的非活动连接；
* 利用单例模式与阻塞队列实现异步的日志系统，记录服务器运行状态；
* 利用RAII机制实现了数据库连接池，减少数据库连接建立与关闭的开销，同时实现了用户注册登录功能。

* 增加logsys,threadpool测试单元(todo: timer, sqlconnpool, httprequest, httpresponse) 

## 环境要求
* Linux
* C++14
* MySql

## 目录树
```
├── bin
│   └── server
├── build
│   └── Makefile
├── code
│   ├── buffer
│   │   ├── buffer.cpp
│   │   ├── buffer.h
│   │   └── readme.md
│   ├── config
│   │   ├── config.cpp
│   │   ├── config.h
│   │   └── readme.md
│   ├── epoll
│   │   ├── epoller.cpp
│   │   ├── epoller.h
│   │   └── readme.md
│   ├── http
│   │   ├── httpconn.cpp
│   │   ├── httpconn.h
│   │   ├── httprequest.cpp
│   │   ├── httprequest.h
│   │   ├── httpresponse.cpp
│   │   ├── httpresponse.h
│   │   └── readme.md
│   ├── log
│   │   ├── blockqueue.h
│   │   ├── log.cpp
│   │   ├── log.h
│   │   └── readme.md
│   ├── main.cpp
│   ├── pool
│   │   ├── readme.md
│   │   ├── sqlconnpool.cpp
│   │   ├── sqlconnpool.h
│   │   ├── sqlconnRAII.h
│   │   └── threadpool.h
│   ├── server
│   │   ├── readme.md
│   │   ├── webserver.cpp
│   │   └── webserver.h
│   └── timer
│       ├── heaptimer.cpp
│       ├── heaptimer.h
│       └── readme.md
├── Makefile
├── readme.md
├── resources
│   ├── 400.html
│   ├── 403.html
│   ├── 404.html
│   ├── 405.html
│   ├── error.html
│   ├── images
│   ├── index.html
│   ├── js
│   ├── login.html
│   ├── picture.html
│   ├── register.html
│   ├── video
│   │   └── xxx.mp4
│   ├── video.html
│   └── welcome.html
├── test
│   ├── Makefile
│   ├── readme.md
│   └── test.cpp
└── webbench-1.5
    ├── Makefile
    ├── socket.c
    ├── webbench
    ├── webbench.c


```


## 项目启动
需要先配置好对应的数据库
```bash
// 建立yourdb库
create database yourdb;

// 创建user表
USE yourdb;
CREATE TABLE user(
    username char(50) NULL,
    passwd char(50) NULL
)ENGINE=InnoDB;

// 添加数据
INSERT INTO user(username, passwd) VALUES('name', 'password');
```

```bash
make
./bin/server
```

## 单元测试
```bash
cd test
make
./test
```

## 压力测试

```bash
./webbench-1.5/webbench -c 10000 -t 10 http://ip:port/
```

![10秒内](E:\Leetcode\Webserver\presstest1.png)

```bash
./webbench-1.5/webbench -c 10000 -t 5 http://ip:port/
```
![5秒](E:\Leetcode\Webserver\presstest2.png)

* 测试环境: Ubuntu:20.0  内存:16G 内核总数：6个 处理器：1个
* QPS 10000+
* webbench使用方法：https://www.cnblogs.com/yinbiao/p/10784450.html

## TODO
* 完善单元测试
* 实现循环缓冲区
* 实现proactor模式

## 致谢
Linux高性能服务器编程，游双著.

[@qinguoyi](https://github.com/qinguoyi/TinyWebServer)
[MARK](https://github.com/markparticle/WebServer)