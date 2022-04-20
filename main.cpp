/*
 * @Author       : llw
 * @Date         : 2022-04-20
 */ 
#include <unistd.h>
#include "server/webserver.h"

int main(){
    /* 守护进程 后台运行 */
    //daemon(1, 0); 

    WebServer server(
        1316, 3, false,         /* 端口 ET模式 优雅退出  */
        12, 6,                  /* 连接池数量 线程池数量 */
        true, 1, 1024           /* 日志开关 日志等级 日志异步队列容量 */
        );
    server.Start();

    return 0;
}
