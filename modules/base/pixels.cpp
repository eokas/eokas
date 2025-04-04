
#include "./pixels.h"
#include <cstring>
#include <algorithm>

namespace eokas {
    
    Pixelmap::Pixelmap()
        : mWidth(0), mHeight(0), mFormat(PixelFormat::Unknown), mData(nullptr) {
    }
    
    Pixelmap::Pixelmap(const Pixelmap& pxmp)
        : mWidth(pxmp.mWidth), mHeight(pxmp.mHeight), mFormat(pxmp.mFormat), mData(nullptr) {
        u32_t size = mWidth * mHeight * _PixelFormatSize(mFormat);
        mData = new u8_t[size];
        memset(mData, 0, size);
        if (pxmp.mData != nullptr) {
            memcpy(mData, pxmp.mData, size);
        }
    }
    
    Pixelmap::Pixelmap(u32_t width, u32_t height, PixelFormat format, void* data)
        : mWidth(width), mHeight(height), mFormat(format), mData(nullptr) {
        u32_t size = mWidth * mHeight * _PixelFormatSize(mFormat);
        mData = new u8_t[size];
        memset(mData, 0, size);
        if (data != nullptr) {
            memcpy(mData, data, size);
        }
    }
    
    Pixelmap::Pixelmap(const Pixelmap& pxmp, u32_t x, u32_t y, u32_t w, u32_t h)
        : mWidth(w), mHeight(h), mFormat(pxmp.mFormat), mData(nullptr) {
        u32_t size = mWidth * mHeight * _PixelFormatSize(mFormat);
        mData = new u8_t[size];
        memset(mData, 0, size);
        if (x < pxmp.mWidth && y < pxmp.mHeight) {
            u32_t cw = std::min(pxmp.mWidth - x, w);
            u32_t ch = std::min(pxmp.mHeight - y, h);
            u32_t row = cw * _PixelFormatSize(mFormat);
            for (u32_t i = 0; i < ch; i++) {
                u8_t* src = (u8_t*) (pxmp.mData) + (y + i) * pxmp.mWidth + x;
                u8_t* dst = (u8_t*) (mData) + i * mWidth;
                memcpy(dst, src, row);
            }
        }
    }
    
    Pixelmap::~Pixelmap() {
        this->clear();
    }
    
    u32_t Pixelmap::width() const {
        return mWidth;
    }
    
    u32_t Pixelmap::height() const {
        return mHeight;
    }
    
    PixelFormat Pixelmap::format() const {
        return mFormat;
    }
    
    void* const Pixelmap::data() const {
        return mData;
    }
    
    Pixelmap Pixelmap::getArea(u32_t x, u32_t y, u32_t w, u32_t h) {
        return Pixelmap(*this, x, y, w, h);
    }
    
    void Pixelmap::setArea(u32_t x, u32_t y, const Pixelmap& pxmp) {
        if (x >= mWidth || y >= mHeight || pxmp.mFormat != mFormat)
            return;
        u32_t cw = std::min(mWidth - x, pxmp.mWidth);
        u32_t ch = std::min(mHeight - y, pxmp.mHeight);
        u32_t row = _PixelFormatSize(mFormat) * cw;
        for (u32_t i = 0; i < ch; i++) {
            u8_t* dst = (u8_t*) mData + (y + i) * mWidth + x;
            u8_t* src = (u8_t*) pxmp.mData + i * cw;
            memcpy(dst, src, row);
        }
    }
    
    void Pixelmap::clear() {
        if (mData != nullptr) {
            delete[]mData;
            mData = nullptr;
        }
        mWidth = 0;
        mHeight = 0;
        mFormat = PixelFormat::Unknown;
    }
    
}
