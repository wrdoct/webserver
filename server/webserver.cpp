
#include "webserver.h"

using namespace std;

WebServer::WebServer(
	int port, int trigMode,
	int connPoolNum, int threadNum,
	bool openLog, int logLevel, int logQueSize) :
	port_(port), isClose_(false),
	threadpool_(new ThreadPool(threadNum)), epoller_(new Epoller())
{
	srcDir_ = getcwd(nullptr, 256); //获取当前的工作路径
	assert(srcDir_);
	strncat(srcDir_, "/resources/", 16);
	HttpConn::userCount = 0;
	HttpConn::srcDir = srcDir_;

	InitEventMode_(trigMode);//设置ET模式
    //初始化套接字
    if(!InitSocket_()) { isClose_ = true;}

    //日志的初始化操作
    if(openLog) {

    }
}

WebServer::~WebServer() {
    close(listenFd_);
    isClose_ = true;
    free(srcDir_);
}

void WebServer::Start() {
    int timeMS = -1;  /* epoll wait的timeout == -1 无事件将阻塞 */
    if(!isClose_) { LOG_INFO("========== Server start =========="); }
    while(!isClose_) {
        int eventCnt = epoller_->Wait(timeMS);//返回值是 检测到有多少个事件发生 

        for(int i = 0; i < eventCnt; i++) {
            /* 处理事件 */
            int fd = epoller_->GetEventFd(i); 
            uint32_t events = epoller_->GetEvents(i); 

            if(fd == listenFd_) {
                DealListen_();
            }
            else if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                assert(users_.count(fd) > 0);
                CloseConn_(&users_[fd]);
            }
            else if(events & EPOLLIN) { 
                assert(users_.count(fd) > 0);
                DealRead_(&users_[fd]);
            }
            else if(events & EPOLLOUT) { 
                assert(users_.count(fd) > 0);
                DealWrite_(&users_[fd]);
            } 
            else {
                LOG_ERROR("Unexpected event");
            }
        }
    }
}

/* Create listenFd */
bool WebServer::InitSocket_() {
    struct sockaddr_in addr;
    if(port_ > 65535 || port_ < 1024) {
        LOG_ERROR("Port:%d error!",  port_);
        return false;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_); 

    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if(listenFd_ < 0) {
        LOG_ERROR("Create socket error!", port_);
        return false;
    }

    int ret;
    int optval = 1;
    /* 端口复用 */
    /* 只有最后一个套接字会正常接收数据。 */
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int)); //设置端口复用
    if(ret == -1) {
        LOG_ERROR("set socket setsockopt error !");
        close(listenFd_);
        return false;
    }

    ret = bind(listenFd_, (struct sockaddr *)&addr, sizeof(addr));
    if(ret < 0) {
        LOG_ERROR("Bind Port:%d error!", port_);
        close(listenFd_);
        return false;
    }

    ret = listen(listenFd_, 6);
    if(ret < 0) {
        LOG_ERROR("Listen port:%d error!", port_);
        close(listenFd_);
        return false;
    }

    ret = epoller_->AddFd(listenFd_,  listenEvent_ | EPOLLIN); //读
    if(ret == 0) { 
        LOG_ERROR("Add listen error!");
        close(listenFd_);
        return false;
    }

    SetFdNonblock(listenFd_); //设置文件描述符非阻塞（epoll）
    LOG_INFO("Server port:%d", port_);

    return true;
}

void WebServer::InitEventMode_(int trigMode) {
	listenEvent_ = EPOLLRDHUP;
	connEvent_ = EPOLLONESHOT | EPOLLRDHUP;
	switch (trigMode)
	{
	case 0:
		break;
	case 1:
		connEvent_ |= EPOLLET;
		break;
	case 2:
		listenEvent_ |= EPOLLET;
		break;
	case 3:
		listenEvent_ |= EPOLLET;
		connEvent_ |= EPOLLET;
		break;
	default:
		listenEvent_ |= EPOLLET;
		connEvent_ |= EPOLLET;
		break;
	}
	HttpConn::isET = (connEvent_ & EPOLLET);
}

void WebServer::AddClient_(int fd, sockaddr_in addr) {
    assert(fd > 0);
    users_[fd].init(fd, addr);//用户数加一，地址，文件描述符，检查缓冲区...
    epoller_->AddFd(fd, EPOLLIN | connEvent_); //向epoll中添加连接的文件描述符（读事件）
    SetFdNonblock(fd); 
    LOG_INFO("Client[%d] in!", users_[fd].GetFd());
}

void WebServer::DealListen_() {
    struct sockaddr_in addr; 
    socklen_t len = sizeof(addr);
    do {
        int fd = accept(listenFd_, (struct sockaddr *)&addr, &len);
        if(fd <= 0) { return;} //
        else if(HttpConn::userCount >= MAX_FD) {
            SendError_(fd, "Server busy!");
            LOG_WARN("Clients is full!");
            return;
        }
        AddClient_(fd, addr); //添加客户端
    } while(listenEvent_ & EPOLLET); //ET模式 
}

void WebServer::DealRead_(HttpConn* client) {
    assert(client);
    //由线程池中的工作线程处理事件————Reactor模式
    threadpool_->AddTask(std::bind(&WebServer::OnRead_, this, client)); //读事件
}

void WebServer::DealWrite_(HttpConn* client) {
    assert(client);
    //由线程池中的工作线程处理事件————Reactor模式
    threadpool_->AddTask(std::bind(&WebServer::OnWrite_, this, client)); //写事件
}

void WebServer::SendError_(int fd, const char*info) {
    assert(fd > 0);
    int ret = send(fd, info, strlen(info), 0);
    if(ret < 0) {
        LOG_WARN("send error to client[%d] error!", fd);
    }
    close(fd);
}

void WebServer::CloseConn_(HttpConn* client) {
    assert(client);
    LOG_INFO("Client[%d] quit!", client->GetFd());
    epoller_->DelFd(client->GetFd());
    client->Close();
}

void WebServer::OnRead_(HttpConn* client){
    assert(client);
    int ret = -1;
    int readErrno = 0;
    ret = client->read(&readErrno);
    if(ret <= 0 && readErrno != EAGAIN) { 
        CloseConn_(client);
        return;
    }
    OnProcess(client); //处理业务逻辑
}

void WebServer::OnWrite_(HttpConn* client){
    
}

void WebServer::OnProcess(HttpConn* client){
    if(client->process()) { //解析完请求，并且响应封装好了
        epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT); //修改文件描述符 改为 写事件
    } else {
        epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLIN); //读事件 
    }
}

int WebServer::SetFdNonblock(int fd){
    assert(fd > 0);

    int flag = fcntl(fd, F_GETFL, 0);
    flag |= O_NONBLOCK;
    return fcntl(fd, F_SETFL, flag);
}