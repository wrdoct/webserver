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
        1316, 3,            /* 端口 ET模式 */
        12, 6              /* 连接池数量 线程池数量 */
        );

    server._Start();

    return 0;
}
