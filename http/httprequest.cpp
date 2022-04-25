#include "httprequest.h"

using namespace std;

const std::unordered_set<std::string> HttpRequest::DEFAULT_HTML{
    "/index", "/compute", "/picture", "/video", "/error",
};

// static const std::unordered_map<std::string, int> HttpRequest::DEFAULT_HTML_TAG{
    
// };

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

    cout<<"解析结果  method_:"<<method_.c_str()<<" path_:"<< path_.c_str()<<" version_:"<< version_.c_str()<<endl;
    return true;
}

std::string HttpRequest::method() const {
    return method_;
}

std::string HttpRequest::path() const{
    return path_;
}

std::string& HttpRequest::path(){
    return path_;
}

std::string HttpRequest::version() const {
    return version_;
}

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
    cout<<"RequestLine Error"<<endl;
    return false;
}

void HttpRequest::ParseHeader_(const string& line) {
    //?前面的元素重复0次或1次  .匹配任意单个字符
    regex patten("^([^:]*): ?(.*)$"); //前面是键 后面是值
    smatch subMatch;
    if(regex_match(line, subMatch, patten)) { 
        header_[subMatch[1]] = subMatch[2]; 
        //cout<<"头"<<header_[subMatch[1]] <<" "<< subMatch[2]<<endl;
    }
    else { //没匹配成功（数据是 回车换行） 就改变状态为 解析请求体
        state_ = BODY;
    }
}

void HttpRequest::ParseBody_(const string& line) {
    body_ = line;
    ParsePost_();
    state_ = FINISH;
    cout<<"Body:"<<line.c_str()<<" len:"<<line.size()<<endl;
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
    if(method_ == "POST" /*&& header_["Content-Type"] == "application/x-www-form-urlencoded"*/) {
        cout<<"解析POST请求"<<endl;
        ParseFromUrlencoded_();
        //int len = std::stoi(header_["Content-Length"]);
        //cout<<len<<endl;
        path_ = "/CGI/compute_.html";
    } 
}

void HttpRequest::ParseFromUrlencoded_() {
    if(body_.size() == 0) { return; }

    string key;
    int value;

    int n = body_.size();
    int i = 0, j = 0;
    for(; i < n; i++) {
        char ch = body_[i];
        switch (ch) {
        case '=':
            key = body_.substr(j, i - j);
            j = i + 1;
            break;
        case '&':
            value = stoi(body_.substr(j, i - j));
            j = i + 1;
            post_[key] = value;
            break;
        default:
            break;
        }
    }
    
    assert(j <= i);
    if(j < i) {
        value = stoi(body_.substr(j, i - j));
        post_[key] = value;
    }
}

std::unordered_map<std::string, int> HttpRequest::Post_(){
    return post_;
}