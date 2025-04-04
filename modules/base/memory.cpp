
#include "./memory.h"
#include <cstring>
#include <cstdlib>
#include <exception>
#include <algorithm>

#if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32
#include <windows.h>
#elif _EOKAS_OS == _EOKAS_OS_MACOS || _EOKAS_OS == _EOKAS_OS_IOS
#include <sys/mman.h>
#else
#include <sys/mman.h>
#endif

namespace eokas {
    /*
    ============================================================================================
    ==== Memory Functions
    ============================================================================================
    */
    void* MemoryUtility::alloc(size_t size)
    {
        void* ptr = std::malloc(size);
        if(ptr != nullptr)
        {
            std::memset(ptr, 0, size);
        }
        return ptr;
    }
    
    void* MemoryUtility::alloc(size_t size, void* data)
    {
        void* ptr = std::malloc(size);
        if(ptr != nullptr && data != nullptr)
        {
            std::memcpy(ptr, data, size);
        }
        return ptr;
    }
    
    void* MemoryUtility::alloc(size_t size, void* data, size_t leng)
    {
        void* ptr = std::malloc(size);
        if(ptr != nullptr)
        {
            std::memset(ptr, 0, size);
            if(data != nullptr)
            {
                size = size < leng ? size : leng;
                std::memcpy(ptr, data, size);
            }
        }
        return ptr;
    }
    
    void* MemoryUtility::realloc(void* ptr, size_t size)
    {
        return std::realloc(ptr, size);
    }
    
    void MemoryUtility::free(void* ptr)
    {
        if(ptr == nullptr)
            return;
        std::free(ptr);
        ptr = nullptr;
    }
    
    void MemoryUtility::clear(void* ptr, size_t size, u8_t value)
    {
        std::memset(ptr, value, size);
    }
    
    void MemoryUtility::copy(void* dst, void* src, size_t size)
    {
        std::memcpy(dst, src, size);
    }
    
    int MemoryUtility::compare(void* ptr1, void* ptr2, size_t size)
    {
        return std::memcmp(ptr1, ptr2, size);
    }
    
#if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32
    
    static u32_t encode_prot(DWORD prot)
    {
        switch (prot)
        {
            case PAGE_READONLY: return 1;
            case PAGE_WRITECOPY: return 2;
            case PAGE_READWRITE: return 3;
            case PAGE_EXECUTE: return 4;
            case PAGE_EXECUTE_READ: return 5;
            case PAGE_EXECUTE_WRITECOPY: return 6;
            case PAGE_EXECUTE_READWRITE: return 7;
        }
        return 0;
    }
    
    void* MemoryUtility::alloc_v(size_t size, u32_t prot)
    {
        DWORD _type = MEM_RESERVE | MEM_COMMIT | MEM_TOP_DOWN;
        DWORD _prot = encode_prot(prot);
        void* ptr = VirtualAlloc(NULL, size, _type, _prot);
        return ptr;
    }
    
    void MemoryUtility::free_v(void* ptr, size_t size)
    {
        VirtualFree(ptr, size, MEM_RELEASE);
    }
    
    u32_t MemoryUtility::prot_v(void* ptr, size_t size, u32_t prot)
    {
        DWORD _old_prot = 0;
        DWORD _new_prot = encode_prot(prot);
        BOOL ret = VirtualProtect(ptr, size, _new_prot, &_old_prot);
        return ret;
    }
    
#else
    
    void* MemoryUtility::alloc_v(size_t size, u32_t prot)
    {
        void *p = mmap(NULL, size, prot, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (p == MAP_FAILED) {
            return nullptr;
        }
        return p;
    }
    
    void MemoryUtility::free_v(void* ptr, size_t size)
    {
        munmap(ptr, size);
    }
    
    u32_t MemoryUtility::prot_v(void* ptr, size_t size, u32_t prot)
    {
        return mprotect(ptr, size, prot);
    }
    
#endif
    
    /*
    ============================================================================================
    ==== MemoryBuffer
    ============================================================================================
    */
    MemoryBuffer::MemoryBuffer()
        :mData(nullptr)
        ,mSize(0)
        ,mIsNewm(false)
    {}
    
    MemoryBuffer::MemoryBuffer(MemoryBuffer&& temp)
        :mData(nullptr)
        ,mSize(0)
        ,mIsNewm(false)
    {
        mData = temp.mData;
        mSize = temp.mSize;
        mIsNewm = temp.mIsNewm;
        temp.mData = nullptr;
        temp.mSize = 0;
        temp.mIsNewm = false;
    }
    
    MemoryBuffer::MemoryBuffer(const MemoryBuffer& other)
        :mData(nullptr)
        ,mSize(other.mSize)
        ,mIsNewm(true)
    {
        mData = MemoryUtility::alloc(other.mSize, other.mData);
    }
    
    MemoryBuffer::MemoryBuffer(size_t size)
        :mData(nullptr)
        ,mSize(size)
        ,mIsNewm(true)
    {
        mData = MemoryUtility::alloc(mSize);
    }
    
    MemoryBuffer::MemoryBuffer(size_t size, void* data, size_t leng)
        :mData(nullptr)
        ,mSize(size)
        ,mIsNewm(true)
    {
        mData = MemoryUtility::alloc(size, data, leng);
    }
    
    MemoryBuffer::MemoryBuffer(void* data, size_t size, bool newm)
        :mData(data)
        ,mSize(size)
        ,mIsNewm(newm)
    {
        if(newm)
        {
            mData = MemoryUtility::alloc(size, data);
        }
    }
    
    MemoryBuffer::~MemoryBuffer()
    {
        if(mIsNewm)
        {
            MemoryUtility::free(mData);
        }
        mData = nullptr;
        mSize = 0;
        mIsNewm = false;
    }
    
    MemoryBuffer& MemoryBuffer::operator=(MemoryBuffer&& temp)
    {
        if(this == &temp)
            return *this;
        // mIsNewm must be assigned
        // the ownership of data is changed.
        mData = temp.mData;
        mSize = temp.mSize;
        mIsNewm = temp.mIsNewm;
        temp.mData = nullptr;
        temp.mSize = 0;
        temp.mIsNewm = false;
        return *this;
    }
    
    MemoryBuffer& MemoryBuffer::operator=(const MemoryBuffer& other)
    {
        if(this == &other)
            return *this;
        // mIsNewm should not be assigned
        // the ownership of data is not changed.
        MemoryUtility::free(mData);
        mData = MemoryUtility::alloc(other.mSize, other.mData);
        mSize = other.mSize;
        return *this;
    }
    
    size_t MemoryBuffer::size() const
    {
        return mSize;
    }
    
    void* const MemoryBuffer::data() const
    {
        return mData;
    }
    
    void MemoryBuffer::fill(void* data)
    {
        if(mData == nullptr)
            return;
        if(data != nullptr)
        {
            MemoryUtility::copy(mData, data, mSize);
        }
        else
        {
            MemoryUtility::clear(mData, mSize, 0);
        }
    }
    
    void MemoryBuffer::fill(void* data, size_t size)
    {
        if(mData == nullptr)
            return;
        size = mSize < size ? mSize : size;
        if(size < mSize)
        {
            MemoryUtility::clear(mData, mSize, 0);
        }
        MemoryUtility::copy(mData, data, size);
    }
    
    void MemoryBuffer::clear()
    {
        if(mData == nullptr)
            return;
        MemoryUtility::clear(mData, mSize, 0);
    }
    
    bool MemoryBuffer::expand(size_t size)
    {
        if(!mIsNewm)
            return false;
        mData = MemoryUtility::realloc(mData, mSize + size);
        mSize = mSize + size;
        return true;
    }
    
    /*
    ============================================================================================
    ==== MemoryStream
    ============================================================================================
    */
    MemoryStream::MemoryStream()
        :mBuffer(new MemoryBuffer(1024))
        ,mIsNewm(true)
        ,mIsOpen(false)
        ,mPos(0)
    {}
    
    MemoryStream::MemoryStream(MemoryStream&& temp)
        :mBuffer(nullptr)
        ,mIsNewm(false)
        ,mIsOpen(false)
        ,mPos(0)
    {
        mBuffer = temp.mBuffer;
        mIsNewm = temp.mIsNewm;
        temp.mBuffer = nullptr;
        temp.mIsNewm = false;
    }
    
    MemoryStream::MemoryStream(const MemoryStream& other)
        :mBuffer(new MemoryBuffer(*other.mBuffer))
        ,mIsNewm(true)
        ,mIsOpen(false)
        ,mPos(0)
    {}
    
    MemoryStream::MemoryStream(MemoryBuffer* memoryBuffer)
        :mBuffer(memoryBuffer)
        ,mIsNewm(false)
        ,mIsOpen(false)
        ,mPos(0)
    {}
    
    MemoryStream::MemoryStream(void* data, size_t size)
        :mBuffer(new MemoryBuffer(data, size, false))
        ,mIsNewm(true)
        ,mIsOpen(false)
        ,mPos(0)
    {}
    
    MemoryStream::~MemoryStream()
    {
        if(mIsNewm)
        {
            delete mBuffer;
        }
        mBuffer = nullptr;
        mIsNewm = false;
        mIsOpen = false;
        mPos = 0;
    }
    
    bool MemoryStream::open()
    {
        mIsOpen = false;
        if(mBuffer != nullptr)
        {
            mIsOpen = true;
            mPos = 0;
        }
        return mIsOpen;
    }
    
    void MemoryStream::close()
    {
        mIsOpen = false;
        mPos = 0;
    }
    
    bool MemoryStream::isOpen() const
    {
        return mIsOpen;
    }
    
    bool MemoryStream::readable() const
    {
        return true;
    }
    
    bool MemoryStream::writable() const
    {
        return true;
    }
    
    bool MemoryStream::eos() const
    {
        return mPos >= mBuffer->size();
    }
    
    size_t MemoryStream::pos() const
    {
        return mPos;
    }
    
    size_t MemoryStream::size() const
    {
        return mBuffer->size();
    }
    
    size_t MemoryStream::read(void* data, size_t size)
    {
        if(!mIsOpen)
            return 0;
    
        u8_t* bufferData = (u8_t*)(mBuffer->data());
        size_t bufferSize = mBuffer->size();
        if(mPos >= bufferSize)
            return 0;
    
        size_t leng = bufferSize - mPos;
        leng = size < leng ? size : leng;
        memcpy(data, bufferData + mPos, leng);
        mPos += leng;
        return leng;
    }
    
    size_t MemoryStream::write(void* data, size_t size)
    {
        if(!mIsOpen)
            return 0;
    
        if(mPos + size > mBuffer->size())
        {
            if (!mBuffer->expand(size * 2))
            {
                return 0;
            }
        }
    
        u8_t* bufferData = (u8_t*)(mBuffer->data());
        memcpy(bufferData + mPos, data, size);
        mPos += size;
        return size;
    }
    
    bool MemoryStream::seek(int offset, int origin) // 0:beg, 1:cur, 2:end
    {
        if(!mIsOpen)
            return false;
        if(origin < 0 || origin > 2)
            return false;
        u8_t* beg = (u8_t*)(mBuffer->data());
        u8_t* cur = (u8_t*)(mBuffer->data()) + mPos;
        u8_t* end = (u8_t*)(mBuffer->data()) + mBuffer->size();
        u8_t* ori = (origin == 0) ? beg : (origin == 1 ? cur : end);
        u8_t* ptr = ori + offset;
        if(ptr < beg || ptr > end)
            return false;
        mPos = ptr - beg;
        return true;
    }
    
    void MemoryStream::flush()
    {}
    
    MemoryStream& MemoryStream::operator=(MemoryStream&& temp)
    {
        if (this == &temp)
            return *this;
        // mIsNewm must be assigned because of the ownership of buffer is changed.
        mBuffer = temp.mBuffer;
        mIsNewm = temp.mIsNewm;
        mIsOpen = temp.mIsOpen;
        mPos = temp.mPos;
        temp.mBuffer = nullptr;
        temp.mIsNewm = false;
        temp.mIsOpen = false;
        temp.mPos = 0;
        return *this;
    }
    
    MemoryStream& MemoryStream::operator=(const MemoryStream& other)
    {
        if (this == &other)
            return *this;
        // mIsNewm should not be assigned because of the ownership of buffer is not changed.
        (*mBuffer) = (*other.mBuffer);
        mIsOpen = other.mIsOpen;
        mPos = other.mPos;
        return *this;
    }
    
    void* MemoryStream::data() const
    {
        return mBuffer->data();
    }
    
}
