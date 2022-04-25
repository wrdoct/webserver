#include "buffer.h"

Buffer::Buffer(int initBuffSize): buffer_(initBuffSize), readPos_(0), writePos_(0) {}

size_t Buffer::WritableBytes() const{
    return buffer_.size() - writePos_;
}  

size_t Buffer::ReadableBytes() const{
    return writePos_ - readPos_;
}

size_t Buffer::PrependableBytes() const{
    return readPos_;
}

const char* Buffer::Peek() const{
    return BeginPtr_() + readPos_;
}

void Buffer::EnsureWriteable(size_t len){
    if(WritableBytes() < len) {
        MakeSpace_(len); //对容器的操作（扩容 或者 拷贝移动数据）
    }
    assert(WritableBytes() >= len);
}

void Buffer::HasWritten(size_t len){
    writePos_ += len;
}

void Buffer::Retrieve(size_t len){
    assert(len <= ReadableBytes());
    readPos_ += len;
}
void Buffer::RetrieveUntil(const char* end){
    assert(Peek() <= end );
    Retrieve(end - Peek()); //移动读指针
}

void Buffer::RetrieveAll(){
    bzero(&buffer_[0], buffer_.size()); 
    readPos_ = 0;
    writePos_ = 0;
} 

const char* Buffer::BeginWriteConst() const{
    return BeginPtr_() + writePos_;
}
char* Buffer::BeginWrite(){
    return BeginPtr_() + writePos_;
}

void Buffer::Append(const std::string& str){
    Append(str.data(), str.length());
}

void Buffer::Append(const char* str, size_t len){
    assert(str);
    EnsureWriteable(len); //这里的len是临时数组中的数据个数
    std::copy(str, str + len, BeginWrite());
    HasWritten(len);
}

ssize_t Buffer::ReadFd(int fd, int* saveErrno){
    char buff[65535]; //临时数组
    struct iovec iov[2]; //定义了一个向量元素  分散的内存
    const size_t writable = WritableBytes();
    /* 分散读， 保证数据全部读完 */ 
    iov[0].iov_base = BeginPtr_() + writePos_;
    iov[0].iov_len = writable; 
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    const ssize_t len = readv(fd, iov, 2); 
    if(len < 0) {
        *saveErrno = errno;
    }
    else if(static_cast<size_t>(len) <= writable) {
        writePos_ += len;
    }
    else {
        writePos_ = buffer_.size();
        Append(buff, len - writable);//把剩下的数据做处理（继续放在当前容器里或者是扩容）
    }

    return len;
}

// ssize_t Buffer::WriteFd(int fd, int* saveErrno){
// }

char* Buffer::BeginPtr_() {
    return &*buffer_.begin();
}

const char* Buffer::BeginPtr_() const {
    return &*buffer_.begin();
}

void Buffer::MakeSpace_(size_t len) {
    if(WritableBytes() + PrependableBytes() < len) { //剩余可写的大小 加 前面可用的空间 小于 临时数组中的长度
        buffer_.resize(writePos_ + len + 1);
    } 
    else { //可以装len长度的数据 就直接将后面的数据拷贝到前面
        size_t readable = ReadableBytes(); 
        std::copy(BeginPtr_() + readPos_, BeginPtr_() + writePos_, BeginPtr_());
        readPos_ = 0; 
        writePos_ = readPos_ + readable; 
        assert(readable == ReadableBytes());
    }
}
