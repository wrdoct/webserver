
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

}

void WebServer::Start() {

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

}


int WebServer::SetFdNonblock(int fd){
    assert(fd > 0);

    int flag = fcntl(fd, F_GETFL, 0);
    flag |= O_NONBLOCK;
    return fcntl(fd, F_SETFL, flag);
}