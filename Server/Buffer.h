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

    ///
    /// Append int64_t using network endian
    ///
    // void appendInt64(int64_t x)
    // {
    //     int64_t be64 = htobe64(x);
    //     append(&be64, sizeof be64);
    // }

    // ///
    // /// Append int32_t using network endian
    // ///
    // void appendInt32(int32_t x)
    // {
    //     int32_t be32 = htobe32(x);
    //     append(&be32, sizeof be32);
    // }

    // void appendInt16(int16_t x)
    // {
    //     int16_t be16 = htobe16(x);
    //     append(&be16, sizeof be16);
    // }

    // void appendInt8(int8_t x)
    // {
    //     append(&x, sizeof x);
    // }

    ///
    /// Read int64_t from network endian
    ///
    /// Require: buf->readableBytes() >= sizeof(int32_t)
    // int64_t readInt64()
    // {
    //     int64_t result = peekInt64();
    //     retrieveInt64();
    //     return result;
    // }

    ///
    /// Read int32_t from network endian
    ///
    /// Require: buf->readableBytes() >= sizeof(int32_t)
    // int32_t readInt32()
    // {
    //     int32_t result = peekInt32();
    //     retrieveInt32();
    //     return result;
    // }

    // int16_t readInt16()
    // {
    //     int16_t result = peekInt16();
    //     retrieveInt16();
    //     return result;
    // }

    // int8_t readInt8()
    // {
    //     int8_t result = peekInt8();
    //     retrieveInt8();
    //     return result;
    // }

    ///
    /// Peek int64_t from network endian
    ///
    /// Require: buf->readableBytes() >= sizeof(int64_t)
    // int64_t peekInt64() const
    // {
    //     assert(readableBytes() >= sizeof(int64_t));
    //     int64_t be64 = 0;
    //     memcpy(&be64, peek(), sizeof be64);
    //     return htobe64(be64);
    // }

    ///
    /// Peek int32_t from network endian
    ///
    /// Require: buf->readableBytes() >= sizeof(int32_t)
    // int32_t peekInt32() const
    // {
    //     assert(readableBytes() >= sizeof(int32_t));
    //     int32_t be32 = 0;
    //     memcpy(&be32, peek(), sizeof be32);
    //     return htobe32(be32);
    // }

    // int16_t peekInt16() const
    // {
    //     assert(readableBytes() >= sizeof(int16_t));
    //     int16_t be16 = 0;
    //     memcpy(&be16, peek(), sizeof be16);
    //     return htobe16(be16);
    // }

    // int8_t peekInt8() const
    // {
    //     assert(readableBytes() >= sizeof(int8_t));
    //     int8_t x = *peek();
    //     return x;
    // }

    ///
    /// Prepend int64_t using network endian
    ///
    // void prependInt64(int64_t x)
    // {
    //     int64_t be64 = sockets::hostToNetwork64(x);
    //     prepend(&be64, sizeof be64);
    // }

    // ///
    // /// Prepend int32_t using network endian
    // ///
    // void prependInt32(int32_t x)
    // {
    //     int32_t be32 = sockets::hostToNetwork32(x);
    //     prepend(&be32, sizeof be32);
    // }

    // void prependInt16(int16_t x)
    // {
    //     int16_t be16 = sockets::hostToNetwork16(x);
    //     prepend(&be16, sizeof be16);
    // }

    // void prependInt8(int8_t x)
    // {
    //     prepend(&x, sizeof x);
    // }

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
        if (writableBytes() + prependableBytes() < len + kPreBytes)
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

#endif