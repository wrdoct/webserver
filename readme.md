# WebServer
用C++实现的高性能WEB服务器

## 功能
* 利用IO复用技术Epoll与线程池实现多线程的Reactor高并发模型；
* 利用正则与状态机解析HTTP请求报文，实现处理静态资源的请求；
* 利用标准库容器封装char，实现自动增长的缓冲区；
* 利用单例模式与阻塞队列实现异步的日志系统，记录服务器运行状态；

## 环境要求
* Linux
* C++14

## 目录树
```
.
├── buffer
│   ├── buffer.cpp
│   ├── buffer.h
│   └── readme.md
├── http
│   ├── httpconn.cpp
│   ├── httpconn.h
│   ├── httprequest.cpp
│   ├── httprequest.h
│   ├── httpresponse.cpp
│   ├── httpresponse.h
│   └── readme.md
├── log
│   ├── blockqueue.h
│   ├── log.cpp
│   ├── log.h
│   └── readme.md
├── main.cpp
├── Makefile
├── readme.md
├── resources
│   ├── compute.html
│   ├── error.html
│   ├── index.html
│   ├── picture
│   │   ├── home.png
│   │   └── img.jpg
│   ├── picture.html
│   ├── video
│   │   └── mp4.mp4
│   └── video.html
└── server
    ├── epoller.cpp
    ├── epoller.h
    ├── readme.md
    ├── threadpool.h
    ├── webserver.cpp
    └── webserver.h

```

## 项目启动
