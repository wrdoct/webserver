# http
## 功能
* 利用正则与状态机解析HTTP请求报文，实现处理静态资源的请求；

## 目录树
```
.
├── httpconn.cpp  读数据、写数据的封装并拿到IP和port
├── httpconn.h
├── httprequest.cpp  解析请求数据
├── httprequest.h
├── httpresponse.cpp  产生并封装响应
├── httpresponse.h
└── readme.md

```