#include "httprequest.h"

using namespace std;

static const std::unordered_set<std::string> DEFAULT_HTML{
    "/index", "/compute", "/picture", "/video", "/error"
};

static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG{
    
};

void HttpRequest::Init() {
    method_ = path_ = version_ = body_ = "";
    state_ = REQUEST_LINE;
    header_.clear();
    post_.clear();
}


bool HttpRequest::parse(Buffer& buff) {
    const char CRLF[] = "\r\n"; //回车符 换行符
    if(buff.ReadableBytes() <= 0) {
        return false;
    }
    while(buff.ReadableBytes() && state_ != FINISH) {
        //获取一行数据，根据\r\n为结束标志
        const char* lineEnd = search(buff.Peek(), buff.BeginWriteConst(), CRLF, CRLF + 2);
        std::string line(buff.Peek(), lineEnd); //封装在line里

        switch(state_)
        {
        case REQUEST_LINE:
            if(!ParseRequestLine_(line)) {
                return false;
            }
            ParsePath_(); 
            break;    
        case HEADERS:
            ParseHeader_(line);
            if(buff.ReadableBytes() <= 2) {
                state_ = FINISH;
            }
            break;
        case BODY:
            ParseBody_(line);
            break;
        default:
            break;
        }
        if(lineEnd == buff.BeginWrite()) { break; }
        buff.RetrieveUntil(lineEnd + 2);
    }

    LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(), version_.c_str());
    return true;
}

std::string HttpRequest::method() const {
    return method_;
}

std::string HttpRequest::path() const{
    return path_;
}

// std::string& HttpRequest::path(){
//     return path_;
// }

std::string HttpRequest::version() const {
    return version_;
}

// std::string HttpRequest::GetPost(const std::string& key) const {
//     assert(key != "");
//     if(post_.count(key) == 1) {
//         return post_.find(key)->second;
//     }
//     return "";
// }

// std::string HttpRequest::GetPost(const char* key) const {
//     assert(key != nullptr);
//     if(post_.count(key) == 1) {
//         return post_.find(key)->second;
//     }
//     return "";
// }

//是否保持 长连接
bool HttpRequest::IsKeepAlive() const {
    if(header_.count("Connection") == 1) {
        return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    }
    return false;
}

bool HttpRequest::ParseRequestLine_(const string& line) {
    // GET / HTTP/1.1  //^开头 ()一组 []满足[]里面的一些字符  *任意多个  $结尾
    regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    smatch subMatch;
    if(regex_match(line, subMatch, patten)) {  //匹配成功就保存在subMatch里面
        method_ = subMatch[1]; // GET
        path_ = subMatch[2]; // /
        version_ = subMatch[3]; // 1.1
        state_ = HEADERS; //状态改变 解析头
        return true;
    }
    LOG_ERROR("RequestLine Error");
    return false;
}

void HttpRequest::ParseHeader_(const string& line) {
    //?前面的元素重复0次或1次  .匹配任意单个字符
    regex patten("^([^:]*): ?(.*)$"); //前面是键 后面是值
    smatch subMatch;
    if(regex_match(line, subMatch, patten)) { 
        header_[subMatch[1]] = subMatch[2]; 
    }
    else { //没匹配成功（数据是 回车换行） 就改变状态为 解析请求体
        state_ = BODY;
    }
}

void HttpRequest::ParseBody_(const string& line) {
    body_ = line;
    ParsePost_();
    state_ = FINISH;
    LOG_DEBUG("Body:%s, len:%d", line.c_str(), line.size());
}

void HttpRequest::ParsePath_() {
    if(path_ == "/") {
        path_ = "/index.html"; 
    }
    else {
        for(auto &item: DEFAULT_HTML) {
            if(item == path_) {
                path_ += ".html";
                break;
            }
        }
    }
}

void HttpRequest::ParsePost_(){
    if(method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded") {
        //CGI服务器

    } 
}

