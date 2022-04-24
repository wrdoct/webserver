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

#include "../log/log.h"
#include "epoller.h"
#include "threadpool.h"
#include "../http/httpconn.h"

class WebServer {
public:
    WebServer(
        int port, int trigMode,
        int connPoolNum, int threadNum);

    ~WebServer();
    
    void _Start();

private:
    bool InitSocket_(); 
    void InitEventMode_(int trigMode);
    void AddClient_(int fd, sockaddr_in addr);
  
    void DealListen_();
    void DealWrite_(HttpConn* client);
    void DealRead_(HttpConn* client);

    void SendError_(int fd, const char*info);
    void CloseConn_(HttpConn* client);

    void OnRead_(HttpConn* client);
    void OnWrite_(HttpConn* client);
    void OnProcess(HttpConn* client);

    static const int MAX_FD = 65536; 

    static int SetFdNonblock(int fd); 

    int port_; 
    bool isClose_; //是否关闭
    int listenFd_; //监听的文件描述符
    char* srcDir_; //资源的目录
    
    uint32_t listenEvent_; 
    uint32_t connEvent_; 
   
    std::unique_ptr<ThreadPool> threadpool_; //线程池
    std::unique_ptr<Epoller> epoller_; //epoll对象
    std::unordered_map<int, HttpConn> users_; //保存的是客户端连接的信息（哈希表：文件描述符-http连接）
};


#endif //WEBSERVER_H