#include "httpconn.h"

using namespace std;

bool HttpConn::isET;
const char* HttpConn::srcDir; 
std::atomic<int> HttpConn::userCount;

HttpConn::HttpConn() { 
    fd_ = -1;
    addr_ = { 0 };
    isClose_ = true; //关闭
}

HttpConn::~HttpConn() { 
    Close(); 
}

void HttpConn::init(int fd, const sockaddr_in& addr) {
    assert(fd > 0);
    userCount++;
    addr_ = addr; 
    fd_ = fd;
    writeBuff_.RetrieveAll(); //检查写缓冲区
    readBuff_.RetrieveAll(); //检查读缓冲区
    isClose_ = false; 
    //LOG_INFO("Client[%d](%s:%d) in, userCount:%d", fd_, GetIP(), GetPort(), (int)userCount);
    cout<<"Client"<<fd_<< GetIP()<<GetPort()<<"in, userCount"<<endl;
}

ssize_t HttpConn::read(int* saveErrno){
    ssize_t len = -1;
    do {
        len = readBuff_.ReadFd(fd_, saveErrno);
        if (len <= 0) {
            break;
        }
    } while (isET);
    return len; 
}

ssize_t HttpConn::write(int* saveErrno){
    ssize_t len = -1;
    do {
        len = writev(fd_, iov_, iovCnt_);  //分散去写 //文件描述符 内存块 内存块数量
        if(len <= 0) {
            *saveErrno = errno;
            break;
        }
        if(iov_[0].iov_len + iov_[1].iov_len  == 0) { break; } /* 传输结束 */
        else if(static_cast<size_t>(len) > iov_[0].iov_len) { 
            iov_[1].iov_base = (uint8_t*) iov_[1].iov_base + (len - iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            if(iov_[0].iov_len) {
                writeBuff_.RetrieveAll();
                iov_[0].iov_len = 0;
            }
        }
        else {
            iov_[0].iov_base = (uint8_t*)iov_[0].iov_base + len; 
            iov_[0].iov_len -= len; 
            writeBuff_.Retrieve(len); 
        }
    } while(isET || ToWriteBytes() > 10240); 
    return len;
}

void HttpConn::Close() {
    response_.UnmapFile(); //解除内存映射
    if(isClose_ == false){
        isClose_ = true;
        userCount--; 
        close(fd_); //关闭文件描述符对应的连接
        //LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", fd_, GetIP(), GetPort(), (int)userCount);
        cout<<"Client"<<fd_<< GetIP()<<GetPort()<<"quit, userCount"<<endl;
    }
}

int HttpConn::GetFd() const {
    return fd_;
}

const char* HttpConn::GetIP() const {
    return inet_ntoa(addr_.sin_addr); //转为诸如“a.b.c.d”的字符串形式
}

int HttpConn::GetPort() const {
    return addr_.sin_port;
} 

struct sockaddr_in HttpConn::GetAddr() const {
    return addr_;
}

//响应的封装
bool HttpConn::process(){
    request_.Init(); 
    if(readBuff_.ReadableBytes() <= 0) { 
        return false; 
    }
    else if(request_.parse(readBuff_)) { //解析并封装响应
        //LOG_DEBUG("%s", request_.path().c_str());
        cout<<"request_.path().c_str()"<<endl;
        //封装响应
        response_.Init(srcDir, request_.path(), request_.IsKeepAlive(), 200); //解析成功 开始封装响应
    } 
    else { //返回错误页面
        response_.Init(srcDir, request_.path(), false, 400);
    }

     response_.MakeResponse(writeBuff_); //响应保存在writeBuff_里面
    /* 响应头 */ //分散的写
    iov_[0].iov_base = const_cast<char*>(writeBuff_.Peek()); 
    iov_[0].iov_len = writeBuff_.ReadableBytes(); 
    iovCnt_ = 1;

    /* 文件 */
    if(response_.FileLen() > 0  && response_.File()) {
        iov_[1].iov_base = response_.File(); //File指向文件映射的指针
        iov_[1].iov_len = response_.FileLen(); //响应体的文件长度
        iovCnt_ = 2; //内存块大小
    }
    //LOG_DEBUG("filesize:%d, %d  to %d", response_.FileLen() , iovCnt_, ToWriteBytes());
    cout<<"filesize: "<<response_.FileLen()<<endl;
    return true;
}