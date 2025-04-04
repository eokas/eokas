
#include "./stream.h"

namespace eokas {
    
    void Stream::read(Stream& stream) {
        Stream& self = *this;
        size_t bsize = 1024;
        u8_t bdata[1024];
        while (!self.eos()) {
            size_t isize = self.read(bdata, bsize);
            size_t osize = stream.write(bdata, isize);
            if (isize < bsize || osize < isize) {
                break;
            }
        }
    }
    
    void Stream::write(Stream& stream) {
        Stream& self = *this;
        size_t bsize = 1024;
        u8_t bdata[1024];
        while (!stream.eos()) {
            size_t isize = stream.read(bdata, bsize);
            size_t osize = self.write(bdata, isize);
            if (isize < bsize || osize < isize) {
                break;
            }
        }
    }
    
    /*
    ============================================================
    */
    DataStream::DataStream(Stream& target)
        : mTarget(&target) {
    }
    
    bool DataStream::open() {
        return mTarget->open();
    }
    
    void DataStream::close() {
        return mTarget->close();
    }
    
    bool DataStream::isOpen() const {
        return mTarget->isOpen();
    }
    
    bool DataStream::readable() const {
        return mTarget->readable();
    }
    
    bool DataStream::writable() const {
        return mTarget->writable();
    }
    
    bool DataStream::eos() const {
        return mTarget->eos();
    }
    
    size_t DataStream::pos() const {
        return mTarget->pos();
    }
    
    size_t DataStream::size() const {
        return mTarget->size();
    }
    
    size_t DataStream::read(void* data, size_t size) {
        return mTarget->read(data, size);
    }
    
    size_t DataStream::write(void* data, size_t size) {
        return mTarget->write(data, size);
    }
    
    bool DataStream::seek(int offset, int origin) // 0:beg, 1:cur, 2:end
    {
        return mTarget->seek(offset, origin);
    }
    
    void DataStream::flush() {
        return mTarget->flush();
    }
    
    Stream& DataStream::target() const {
        return *mTarget;
    }
    
    void DataStream::bind(Stream& stream) {
        this->mTarget = &stream;
    }
    
}
