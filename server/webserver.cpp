#include "webserver.h"

using namespace std;

WebServer::WebServer(
	int port, int trigMode,
	int connPoolNum, int threadNum) :
	port_(port), isClose_(false),
	threadpool_(new ThreadPool(threadNum)), epoller_(new Epoller())
{
	srcDir_ = getcwd(nullptr, 256); //获取当前的工作路径
	assert(srcDir_);
	strncat(srcDir_, "/resources/", 16);
    //strncat(srcDir_, "/resources", 16);
	HttpConn::userCount = 0;
	HttpConn::srcDir = srcDir_;

	InitEventMode_(trigMode);//设置ET模式
    //初始化套接字
    if(!InitSocket_()) { isClose_ = true;}
}

WebServer::~WebServer() {
    close(listenFd_);
    isClose_ = true;
    free(srcDir_);
}

void WebServer::_Start() {
    int timeMS = -1;  /* epoll wait的timeout == -1 无事件将阻塞 */
    if(!isClose_) { cout << "========== Server start =========="<<endl; }
    while(!isClose_) {
        int eventCnt = epoller_->Wait(timeMS);//返回值是 检测到有多少个事件发生 
        //cout<<"eventCnt:"<<eventCnt<<endl;
        for(int i = 0; i < eventCnt; i++) {
            /* 处理事件 */
            int fd = epoller_->GetEventFd(i); 
            uint32_t events = epoller_->GetEvents(i);
            
            cout<<"判断事件的文件描述符"<<endl;
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
                cout<<"Unexpected event"<<endl;
            }
        }
    }
}

/* Create listenFd */
bool WebServer::InitSocket_() {
    struct sockaddr_in addr;
    if(port_ > 65535 || port_ < 1024) {
        cout<<"Port:"<<port_<<"error!"<<endl;
        return false;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_); 

    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if(listenFd_ < 0) {
        cout<<"Create socket error!"<<" "<<port_<<endl;
        return false;
    }

    int ret;
    int optval = 1;
    /* 端口复用 */
    /* 只有最后一个套接字会正常接收数据。 */
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int)); //设置端口复用
    if(ret == -1) {
        cout<<"set socket setsockopt error !"<<endl;
        close(listenFd_);
        return false;
    }

    ret = bind(listenFd_, (struct sockaddr *)&addr, sizeof(addr));
    if(ret < 0) {
        cout<<"Bind Port:"<<port_<<"error!"<<endl;
        close(listenFd_);
        return false;
    }

    ret = listen(listenFd_, 6);
    if(ret < 0) {
        cout<<"Listen Port:"<<port_<<"error!"<<endl;
        close(listenFd_);
        return false;
    }

    ret = epoller_->AddFd(listenFd_,  listenEvent_ | EPOLLIN); //读
    if(ret == 0) { 
        cout<<"Add listen error!"<<endl;
        close(listenFd_);
        return false;
    }

    SetFdNonblock(listenFd_); //设置文件描述符非阻塞（epoll）
    cout<<"Server Port:"<<port_<<endl;
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
    cout<<"AddClient_ "<<users_[fd].GetFd() <<" in!"<<endl;
}

void WebServer::DealListen_() {
    cout<<"开始处理监听"<<endl;
    struct sockaddr_in addr; 
    socklen_t len = sizeof(addr);
    do {
        int fd = accept(listenFd_, (struct sockaddr *)&addr, &len);
        if(fd <= 0) { return;} //
        else if(HttpConn::userCount >= MAX_FD) {
            SendError_(fd, "Server busy!");
            cout<<"Clients is full!"<<endl;
            return;
        }
        AddClient_(fd, addr); //添加客户端
    } while(listenEvent_ & EPOLLET); //ET模式 
}

void WebServer::DealRead_(HttpConn* client) {
    cout<<"开始处理读事件"<<endl;
    assert(client);
    //由线程池中的工作线程处理事件————Reactor模式
    threadpool_->AddTask(std::bind(&WebServer::OnRead_, this, client)); //读事件
}

void WebServer::DealWrite_(HttpConn* client) {
    cout<<"开始处理写事件"<<endl;
    assert(client);
    //由线程池中的工作线程处理事件————Reactor模式
    threadpool_->AddTask(std::bind(&WebServer::OnWrite_, this, client)); //写事件
}

void WebServer::SendError_(int fd, const char*info) {
    assert(fd > 0);
    int ret = send(fd, info, strlen(info), 0);
    if(ret < 0) {
        cout<<"send error to client"<<fd<<" error!"<<endl;
    }
    close(fd);
}

void WebServer::CloseConn_(HttpConn* client) {
    assert(client);
    cout<<"Client:"<<client->GetFd()<<" quit!"<<endl;
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
    assert(client);
    int ret = -1;
    int writeErrno = 0;
    ret = client->write(&writeErrno);
    if(client->ToWriteBytes() == 0) {
        /* 传输完成 */
        if(client->IsKeepAlive()) {
            OnProcess(client);
            return;
        }
    }
    else if(ret < 0) {
        if(writeErrno == EAGAIN) {
            /* 继续传输 */
            epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT); 
            return;
        }
    }
    //sleep(5000);
    CloseConn_(client);
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
