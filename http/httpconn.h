#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <sys/types.h>
#include <sys/uio.h>     // readv/writev
#include <arpa/inet.h>   // sockaddr_in
#include <stdlib.h>      
#include <errno.h>  

#include "../log/log.h"
#include "../buffer/buffer.h"
#include "httprequest.h"
#include "httpresponse.h"

class HttpConn {
public:
    HttpConn();

    ~HttpConn();

    void init(int sockFd, const sockaddr_in& addr);

    ssize_t read(int* saveErrno);

    ssize_t write(int* saveErrno);

    void Close();

    int GetFd() const;

    const char* GetIP() const;
    
    int GetPort() const;
   
    sockaddr_in GetAddr() const;
    
    bool process();

    int ToWriteBytes() { 
        return iov_[0].iov_len + iov_[1].iov_len; 
    }

    bool IsKeepAlive() const {
        return request_.IsKeepAlive();
    }

    static bool isET;
    static const char* srcDir; 
    static std::atomic<int> userCount; 
    
private:
   
    int fd_;
    struct sockaddr_in addr_;

    bool isClose_; 
    
    int iovCnt_;
    struct iovec iov_[2]; //定义了一个向量元素  分散的内存
    
    Buffer readBuff_; // 读（请求）缓冲区 
    Buffer writeBuff_; // 写（响应）缓冲区 

    HttpRequest request_; 
    HttpResponse response_; 
};


#endif //HTTP_CONN_H
