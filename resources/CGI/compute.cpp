#include <iostream>
#include <string>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

int main( int argc, char **argv ) {
    int _digit, digit_, accp_fd;
    sscanf(argv[0], "%d+%d,%d", &_digit, &digit_, &accp_fd);
    
    int t = _digit+digit_;
    string response = "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\nContent-Type: text/html;charset=utf-8\r\n\r\n";
    string body = "<html><head><title>niliushall's CGI</title></head>";
    body += "<body><p>The result is " + to_string(_digit) + "+" + to_string(digit_) + " = " + to_string(t);
    body += "</p></body></html>";
    response += body;
    dup2(accp_fd, STDOUT_FILENO);
}