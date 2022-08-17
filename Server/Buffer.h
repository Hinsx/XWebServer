#ifndef XBUFFER_H
#define XBUFFER_H

#include <vector>
#include <assert.h>
#include <string>
#include <string.h>
#include <algorithm>

/*
封装从socket读取/输出数据的细节，包括寻找CLR
使用vector作为核心，使用readerIndex和writerIndex，将buffer分为三部分
|       prependable      ↓       readable        ↓         writable        |
                    readerIndex              writerIndex
其中
A+B+C=vector.size()
A的存在允许在填充http回应时，根据内容长度，再来填写头部长度字段
B是已读取但仍未解析的数据
C是剩余可填充数据，C的存在允许在解析http请求时，若长度不够分析，则等待下一次数据append至B末尾
*/
class Buffer
{
    static const int kPreBytes = 8;
    static const int kInitialSize = 1024;

public:
    Buffer(int initialSize = kInitialSize) : buffer_(kPreBytes + initialSize),
                                             readerIndex_(kPreBytes),
                                             writerIndex_(kPreBytes)
    {
    }


    size_t readableBytes() const
    {
        return writerIndex_ - readerIndex_;
    }

    size_t writableBytes() const
    {
        return buffer_.size() - writerIndex_;
    }

    size_t prependableBytes() const
    {
        return readerIndex_;
    }

    const char *peek() const
    {
        return begin() + readerIndex_;
    }

    //寻找\r\n
    const char *findCRLF() const
    {
        // FIXME: replace with memmem()?
        const char *crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF + 2);
        return crlf == beginWrite() ? NULL : crlf;
    }

    const char *findCRLF(const char *start) const
    {
        assert(peek() <= start);
        assert(start <= beginWrite());
        // FIXME: replace with memmem()?
        const char *crlf = std::search(start, beginWrite(), kCRLF, kCRLF + 2);
        return crlf == beginWrite() ? NULL : crlf;
    }

    const char *findEOL() const
    {
        const void *eol = memchr(peek(), '\n', readableBytes());
        return static_cast<const char *>(eol);
    }

    const char *findEOL(const char *start) const
    {
        assert(peek() <= start);
        assert(start <= beginWrite());
        const void *eol = memchr(start, '\n', beginWrite() - start);
        return static_cast<const char *>(eol);
    }

    // retrieve returns void, to prevent
    // string str(retrieve(readableBytes()), readableBytes());
    // the evaluation of two functions are unspecified
    void retrieve(size_t len)
    {
        assert(len <= readableBytes());
        if (len < readableBytes())
        {
            readerIndex_ += len;
        }
        else
        {
            retrieveAll();
        }
    }

    void retrieveUntil(const char *end)
    {
        assert(peek() <= end);
        assert(end <= beginWrite());
        retrieve(end - peek());
    }

    void retrieveInt64()
    {
        retrieve(sizeof(int64_t));
    }

    void retrieveInt32()
    {
        retrieve(sizeof(int32_t));
    }

    void retrieveInt16()
    {
        retrieve(sizeof(int16_t));
    }

    void retrieveInt8()
    {
        retrieve(sizeof(int8_t));
    }

    void retrieveAll()
    {
        readerIndex_ = kPreBytes;
        writerIndex_ = kPreBytes;
    }

    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }

    std::string retrieveAsString(size_t len)
    {
        assert(len <= readableBytes());
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }

    void append(const char * /*restrict*/ data, size_t len)
    {
        ensureWritableBytes(len);
        std::copy(data, data + len, beginWrite());
        hasWritten(len);
    }

    void append(const char * /*restrict*/ data)
    {
        append(data,strlen(data));
    }
    void append(const std::string& str){
        ensureWritableBytes(str.size());
        std::copy(str.begin(), str.end(), beginWrite());
        hasWritten(str.size());
    }

    // void append(const void * /*restrict*/ data, size_t len)
    // {
    //     append(static_cast<const char *>(data), len);
    // }

    void ensureWritableBytes(size_t len)
    {
        if (writableBytes() < len)
        {
            makeSpace(len);
        }
        assert(writableBytes() >= len);
    }

    char *beginWrite()
    {
        return begin() + writerIndex_;
    }

    const char *beginWrite() const
    {
        return begin() + writerIndex_;
    }

    void hasWritten(size_t len)
    {
        assert(len <= writableBytes());
        writerIndex_ += len;
    }

    void unwrite(size_t len)
    {
        assert(len <= readableBytes());
        writerIndex_ -= len;
    }

    void prepend(const char * /*restrict*/ data, size_t len)
    {
        assert(len <= prependableBytes());
        readerIndex_ -= len;
        std::copy(data, data + len, begin() + readerIndex_);
    }

    size_t internalCapacity() const
    {
        return buffer_.capacity();
    }

    ssize_t readFd(int fd, int *savedErrno);

private:
    char *begin()
    {
        return &*buffer_.begin();
    }

    const char *begin() const
    {
        return &*buffer_.begin();
    }

    //小优化，避免vector重新分配空间
    void makeSpace(size_t len)
    {
        if (writableBytes() + prependableBytes() - kPreBytes< len )
        {
            // FIXME: move readable data
            buffer_.resize(writerIndex_ + len);
        }
        else
        {
            // move readable data to the front, make space inside buffer
            assert(kPreBytes < readerIndex_);
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_,
                      begin() + writerIndex_,
                      begin() + kPreBytes);
            readerIndex_ = kPreBytes;
            writerIndex_ = readerIndex_ + readable;
            assert(readable == readableBytes());
        }
    }

    std::vector<char> buffer_;
    int readerIndex_;
    int writerIndex_;

    static const char kCRLF[];
};
//发送请求数据流封装，辅助实现零拷贝，允许大文件断点续传
class SendMsg
{
    struct SendFile
    {
        private:
        //std::string fileName_;
        int filefd_;
        off_t nwrote_;
        size_t fileSize_;
        public:
        // SendFile(const std::string& fileName,size_t size):
        // fileName_(fileName),
        // nwrote_(0),
        // fileSize_(size)
        // {}
        SendFile(int filefd,size_t size):
        filefd_(filefd),
        nwrote_(0),
        fileSize_(size)
        {}
        void retrieve(size_t n){nwrote_+=n;}
        off_t* nworte(){return &nwrote_;}
        size_t fileSize(){return fileSize_;}
        // const std::string& filename()const{return fileName_;}
        int filefd(){return filefd_;}
        bool faileDone(){return nwrote_==static_cast<size_t>(fileSize_);}
    };
    public:
    // SendMsg(const std::string& fileName,size_t size=0):
    // file_(fileName,size),
    // done_(false)
    // {}
    SendMsg(int filefd,size_t size=0):
    file_(filefd,size),
    done_(false)
    {}
    Buffer* getBuffer()
    {
        return &buffer_;
    }
    SendFile* getSendFile()
    {
        return &file_;
    }
    bool isWriteAll(){return done_;}
    //向socket写入数据，返回写入的字节数
    size_t writeToSocket(int fd);
    private:
    Buffer buffer_;
    SendFile file_;
    //标识该信息是否已经写入socket缓冲区
    bool done_;
};

#endif