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

}

void Buffer::HasWritten(size_t len){

}

void Buffer::Retrieve(size_t len){

}
void Buffer::RetrieveUntil(const char* end){

}

void Buffer::RetrieveAll(){

} 
std::string Buffer::RetrieveAllToStr(){

}

const char* Buffer::BeginWriteConst() const{
    return BeginPtr_() + writePos_;
}
char* Buffer::BeginWrite(){
    return BeginPtr_() + writePos_;
}

void Buffer::Append(const std::string& str){

}
void Buffer::Append(const char* str, size_t len){

}
void Buffer::Append(const void* data, size_t len){

}
/*void Buffer::Append(const Buffer& buff){

}*/

ssize_t Buffer::ReadFd(int fd, int* Errno){

}
ssize_t Buffer::WriteFd(int fd, int* Errno){

}

char* Buffer::BeginPtr_() {
    return &*buffer_.begin();
}

const char* Buffer::BeginPtr_() const {
    return &*buffer_.begin();
}

void Buffer::MakeSpace_(size_t len) {
    if(WritableBytes() + PrependableBytes() < len) { //剩余可写的大小 加 前面可用的空间 小于 临时数组中的长度
        buffer_.resize(writePos_ + len + 1); //调整容器大小
    } 
    else { //可以装len长度的数据 就直接将后面的数据拷贝到前面
        size_t readable = ReadableBytes(); //可以读的数据的大小
        std::copy(BeginPtr_() + readPos_, BeginPtr_() + writePos_, BeginPtr_());//把后面的未读数据拷贝到前面 //std::copy(start, end, std::back_inserter(container)); 
        readPos_ = 0; //恢复读的初始位置为0
        writePos_ = readPos_ + readable; //已写了的位置等于 读的初始位置0 加上 之前剩余未读（可以读）的数据
        assert(readable == ReadableBytes());
    }
}
