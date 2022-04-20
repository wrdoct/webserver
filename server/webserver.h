/*
 * @Author       : llw
 * @Date         : 2022-04-20
 */ 
#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <string>
#include <unordered_map>
#include <fcntl.h>    
#include <unistd.h>     
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "epoller.h"
#include "threadpool.h"

class WebServer {
public:
    WebServer(
        int port, int trigMode, bool OptLinger, 
        int connPoolNum, int threadNum,
        bool openLog, int logLevel, int logQueSize);

    ~WebServer();
    void Start();

private:
    bool InitSocket_(); 
    void InitEventMode_(int trigMode);
    void AddClient_(int fd, sockaddr_in addr);
  
    void DealListen_();
    // void DealWrite_(HttpConn* client);
    // void DealRead_(HttpConn* client);

    // void SendError_(int fd, const char*info);
    // void CloseConn_(HttpConn* client);

    // void OnRead_(HttpConn* client);
    // void OnWrite_(HttpConn* client);
    // void OnProcess(HttpConn* client);

    static const int MAX_FD = 65536; //最大的文件描述符的个数

    static int SetFdNonblock(int fd); //设置文件描述符非阻塞

    int port_; //端口
    bool openLinger_; //是否打开优雅关闭
    bool isClose_; //是否关闭
    int listenFd_; //监听的文件描述符
    char* srcDir_; //资源的目录
    
    uint32_t listenEvent_; //监听的文件描述符的事件
    uint32_t connEvent_; //连接的文件描述符的事件
   
    std::unique_ptr<ThreadPool> threadpool_; //线程池
    std::unique_ptr<Epoller> epoller_; //epoll对象
    //std::unordered_map<int, HttpConn> users_; //保存的是客户端连接的信息（哈希表：文件描述符-http连接）
};


#endif //WEBSERVER_H