/*
 * @Author       : llw
 * @Date         : 2022-04-20
 */

#include "webserver.h"

using namespace std;

WebServer::WebServer(
    int port, int trigMode, bool OptLinger,
    int connPoolNum, int threadNum,
    bool openLog, int logLevel, int logQueSize) : port_(port), openLinger_(OptLinger), isClose_(false),
                                                  threadpool_(new ThreadPool(threadNum)), epoller_(new Epoller())
{

        srcDir_ = getcwd(nullptr, 256); //获取当前的工作路径
        assert(srcDir_);
        strncat(srcDir_, "/resources/", 16);


        InitEventMode_(trigMode);
}

WebServer::~WebServer()
{
}
