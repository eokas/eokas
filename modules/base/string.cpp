
#include "./string.h"
#include <cstring>
#include <algorithm>

namespace eokas {
    
    class StringPool {
    public:
        static StringPool& instance() {
            static StringPool sInstance;
            return sInstance;
        }
        
        ~StringPool() {
            if (!mStrings.empty()) {
                auto iter = mStrings.begin();
                while (iter != mStrings.end()) {
                    char* ptr = (char*) (*iter);
                    if (ptr != nullptr) {
                        delete[]ptr;
                    }
                    ++iter;
                }
                mStrings.clear();
            }
        }
        
        char* get() {
            char* ptr = nullptr;
            if (mStrings.empty()) {
                ptr = new char[_STRING_MIDDLE_LENGTH + 1];
            } else {
                ptr = (char*) mStrings.front();
                mStrings.pop_front();
            }
            memset(ptr, 0, _STRING_MIDDLE_LENGTH + 1);
            return ptr;
        }
        
        void put(char* ptr) {
            if (ptr == nullptr)
                return;
            mStrings.push_back(ptr);
        }
    
    private:
        std::list<char*> mStrings;
    };
    
    const size_t String::npos = (size_t) (-1);
    const String String::empty = "";
    const String String::zero = "0";
    const String String::one = "1";
    const String String::trueValue = "true";
    const String String::falseValue = "false";
    
    char String::measure(size_t len) {
        if (len >= _STRING_MIDDLE_LENGTH)
            return 2;
        if (len >= _STRING_LITTLE_LENGTH)
            return 1;
        return 0;
    }
    
    size_t String::predict(size_t len) {
        if (len >= _STRING_MIDDLE_LENGTH)
            return (size_t) (len * 2);
        if (len >= _STRING_LITTLE_LENGTH)
            return _STRING_MIDDLE_LENGTH;
        return _STRING_LITTLE_LENGTH;
    }
    /*
    ============================================================================================
    ==== String -- codec functions
    ============================================================================================
    */
    /*
    UCS-4 : 32b(31b used) 0xxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx
    byte[0] is named group (128 groups in total)
    byte[1] is named plane (256 planes per group)
    byte[2] is named row (256 rows per plane)
    byte[3] is named cell (256 cells per row)
    <group[0], plane[0]> is named Basic Multilingual Plane (BMP)
    UCS-2 : 16b, equals UCS-4 BMP
    */
    /*
    Zero Width No-Break Space (BOM, Byte Order Mark)
    Unicode BOM : FEFF
    FE FF : Big Endian
    FF FE : Little Endian
    UTF8 BOM : EF BB BF
    */
    /*
    Unicode to UTF8
    U-00000000 - U-0000007F: 0xxxxxxx
    U-00000080 - U-000007FF: 110xxxxx 10xxxxxx
    U-00000800 - U-0000FFFF: 1110xxxx 10xxxxxx 10xxxxxx
    U-00010000 - U-001FFFFF: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    U-00200000 - U-03FFFFFF: 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
    U-04000000 - U-7FFFFFFF: 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
    */
    MBString String::unicodeToUtf8(const WCString& unicodeStr, bool bom) {
        std::stringstream utf8;
        if (bom) {
            utf8 << "\xef\xbb\xbf"; // utf8 bom is EF BB BF
        }
        
        const wchar_t* pos = unicodeStr.c_str();
        size_t length = unicodeStr.length();
        size_t count = 0;
        // check and ignore bom in unicodeStr
        if (length >= 1 && pos[0] == 0xFEFF) {
            count += 1;
            pos += 1;
        }
        while (count < length) {
            if (*pos < 0x80) {
                char c1 = *pos & 0x7f;
                utf8 << c1;
            } else if (*pos < 0x800) {
                char c1 = ((*pos >> 6) & 0x1f) | 0xc0;
                char c2 = (*pos & 0x3f) | 0x80;
                utf8 << c1 << c2;
            } else if (*pos < 0x10000) {
                char c1 = ((*pos >> 12) & 0x0f) | 0xe0;
                char c2 = ((*pos >> 6) & 0x3f) | 0x80;
                char c3 = (*pos & 0x3f) | 0x80;
                utf8 << c1 << c2 << c3;
            }
#if (_EOKAS_UCS == _EOKAS_UCS_4)
            else if(*pos < 0x200000)
            {
                char c1 = ((*pos >> 18) & 0x07) | 0xf0;
                char c2 = ((*pos >> 12) & 0x3f) | 0x80;
                char c3 = ((*pos >> 6) & 0x3f) | 0x80;
                char c4 = (*pos & 0x3f) | 0x80;
                utf8 << c1 << c2 << c3 << c4;
            }
            else if(*pos < 0x4000000)
            {
                char c1 = ((*pos >> 24) & 0x03) | 0xf8;
                char c2 = ((*pos >> 18) & 0x3f) | 0x80;
                char c3 = ((*pos >> 12) & 0x3f) | 0x80;
                char c4 = ((*pos >> 6) & 0x3f) | 0x80;
                char c5 = (*pos & 0x3f) | 0x80;
                utf8 << c1 << c2 << c3 << c4 << c5;
            }
            else if(*pos < 0x80000000)
            {
                char c1 = ((*pos >> 30) & 0x01)|0xfc;
                char c2 = ((*pos >> 24) & 0x3f)|0x80;
                char c3 = ((*pos >> 18) & 0x3f)|0x80;
                char c4 = ((*pos >> 12) & 0x3f)|0x80;
                char c5 = ((*pos >> 6) & 0x3f)|0x80;
                char c6 = (*pos & 0x3f)|0x80;
                utf8 << c1 << c2 << c3 << c4 << c5 << c6;
            }
#endif//(_EOKAS_UCS == _EOKAS_UCS_4)
            count += 1;
            pos += 1;
        }
        
        return utf8.str();
    }
    
    WCString String::utf8ToUnicode(const MBString& utf8Str, bool bom) {
        std::wstringstream unicode;
        if (bom) {
            unicode << 0xFEFF; // unicode bom
        }
        const char* pos = utf8Str.c_str();
        size_t length = utf8Str.length();
        size_t count = 0;
        // check and ignore bom in utf8Str
        if (length >= 3 && int(pos[0]) == 0xEF && int(pos[1]) == 0xBB && int(pos[2]) == 0xBF) {
            count += 3;
            pos += 3;
        }
        while (count < length) {
            wchar_t wc = L'\0';
            u8_t c = *pos;
            if (c < 0xC0) {
                count += 1;
                if (count <= length) {
                    wc = *pos;
                    pos += 1;
                }
            } else if (c < 0xE0) {
                count += 2;
                if (count <= length) {
                    wc = (pos[0] & 0x1F) << 6;
                    wc |= (pos[1] & 0x3F);
                    pos += 2;
                }
            } else if (c < 0xF0) {
                count += 3;
                if (count <= length) {
                    wc = (pos[0] & 0x0F) << 12;
                    wc |= (pos[1] & 0x3F) << 6;
                    wc |= (pos[2] & 0x3F);
                    pos += 3;
                }
            }
#if (_EOKAS_UCS == _EOKAS_UCS_4)
            else if(c < 0xF8)
            {
                count += 4;
                if(count <= length)
                {
                    wc = (*pos[0] & 0x07) << 18;
                    wc |= (*pos[1] & 0x3F) << 12;
                    wc |= (*pos[2] & 0x3F) << 6;
                    wc |= (*pos[3] & 0x3F);
                    pos += 4;
                }
            }
            else if(c < 0xFC)
            {
                count += 5;
                if(count <= length)
                {
                    wc = (*pos[0] & 0x03) << 24;
                    wc |= (*pos[1] & 0x3F) << 18;
                    wc |= (*pos[2] & 0x3F) << 12;
                    wc |= (*pos[3] & 0x3F) << 6;
                    wc |= (*pos[4] & 0x3F);
                    pos += 5;
                }
            }
            else
            {
                count += 6;
                if(count <= length)
                {
                    wc = (*pos[0] & 0x01) << 30;
                    wc |= (*pos[1] & 0x3F) << 24;
                    wc |= (*pos[2] & 0x3F) << 18;
                    wc |= (*pos[3] & 0x3F) << 12;
                    wc |= (*pos[4] & 0x3F) << 6;
                    wc |= (*pos[5] & 0x3F);
                    pos += 6;
                }
            }
#endif//(_EOKAS_UCS == _EOKAS_UCS_4)
            unicode << wc;
        }
        
        return unicode.str();
    }
    
    static u8_t hexchars[] = "0123456789ABCDEF";
    
    static int str_htoi_2(const char* s) {
        int value = 0;
        int c = 0;
        
        c = ((u8_t*) s)[0];
        if (isupper(c))
            c = tolower(c);
        value = (c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10) * 16;
        
        c = ((u8_t*) s)[1];
        if (isupper(c))
            c = tolower(c);
        value += c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10;
        
        return value;
    }
    
    String String::encodeURL(const String& str) {
        std::stringstream stream;
        const char* ptr = str.cstr();
        while (*ptr != '\0') {
            char c = *ptr++;
            if (c == ' ')
                stream << '+';
            else if (c == '-' || c == '.' || c == '_' || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
                stream << c;
            } else {
                stream << '%' << hexchars[c >> 4] << hexchars[c & 15];
            }
        }
        return stream.str();
    }
    
    String String::decodeURL(const String& str) {
        std::stringstream stream;
        const char* ptr = str.cstr();
        while (*ptr != '\0') {
            if (*ptr == '+') {
                stream << ' ';
                ptr++;
            } else if (*ptr == '%' && isxdigit((int) *(ptr + 1)) && isxdigit((int) *(ptr + 2))) {
                stream << (char) str_htoi_2(ptr + 1);
                ptr += 2;
            } else {
                stream << *ptr++;
            }
        }
        return stream.str();
    }
    
    String String::format(const char* fmt, ...) {
        String result;
        _FormatVA(result, fmt);
        return result;
    }
    
    String String::repeat(const String& str, size_t n) {
        if (n == 0) return "";
        std::stringstream stream;
        for (size_t i = 0; i < n; i++) {
            stream << str.mData;
        }
        return stream.str();
    }
    
    String String::join(const StringVector& segments, const String& delim) {
        if (segments.empty())
            return "";
        std::stringstream stream;
        auto iter = segments.begin();
        stream << iter->mData;
        ++iter;
        while (iter != segments.end()) {
            stream << delim.mData;
            stream << iter->mData;
            ++iter;
        }
        return stream.str();
    }
    
    String String::join(const StringMap& segments, const String& conn, const String& delim) {
        if (segments.empty())
            return "";
        std::stringstream stream;
        auto iter = segments.begin();
        stream << iter->first.mData << conn.mData << iter->second.mData;
        ++iter;
        while (iter != segments.end()) {
            stream << delim.mData;
            stream << iter->first.mData << conn.mData << iter->second.mData;
            ++iter;
        }
        return stream.str();
    }
    
    String::String(char chr, size_t len)
        : mData(mValue), mSize(0), mCapacity(_STRING_LITTLE_LENGTH), mMetric(0) {
        mValue[0] = '\0';
        len = chr != '\0' ? len : 0;
        if (len > 0) {
            mMetric = String::measure(len);
            if (mMetric == 1) {
                mData = StringPool::instance().get();
                mCapacity = _STRING_MIDDLE_LENGTH;
            } else if (mMetric == 2) {
                mCapacity = String::predict(len + 1);
                mData = new char[mCapacity];
            }
            memset(mData, chr, len);
            mData[len] = '\0';
            mSize = len;
        }
    }
    
    String::String(const char* mbcstr, size_t len)
        : mData(mValue), mSize(0), mCapacity(_STRING_LITTLE_LENGTH), mMetric(0) {
        mValue[0] = '\0';
        if (mbcstr != nullptr) {
            len = std::min(len, strlen(mbcstr));
            if (len > 0) {
                mMetric = String::measure(len);
                if (mMetric == 1) {
                    mData = StringPool::instance().get();
                    mCapacity = _STRING_MIDDLE_LENGTH;
                } else if (mMetric == 2) {
                    mCapacity = String::predict(len + 1);
                    mData = new char[mCapacity];
                }
                memcpy(mData, mbcstr, len);
                mData[len] = '\0';
                mSize = len;
            }
        }
    }
    
    String::String(const MBString& mbstr)
        : mData(mValue), mSize(0), mCapacity(_STRING_LITTLE_LENGTH), mMetric(0) {
        new(this)String(mbstr.c_str());
    }
    
    String::String(const WCString& wcstr)
        : mData(mValue), mSize(0), mCapacity(_STRING_LITTLE_LENGTH), mMetric(0) {
        new(this)String(String::unicodeToUtf8(wcstr, false).c_str());
    }
    
    String::String(const String& other)
        : mData(mValue), mSize(0), mCapacity(_STRING_LITTLE_LENGTH), mMetric(0) {
        new(this)String(other.mData);
    }
    
    String::String(String&& other)
        : mData(mValue), mSize(other.mSize), mCapacity(other.mCapacity), mMetric(other.mMetric) {
        mValue[0] = '\0';
        size_t len = mSize;
        if (mMetric == 0) {
            memcpy(mData, other.mData, len);
            mData[len] = '\0';
        } else if (mMetric == 1 || mMetric == 2) {
            mData = other.mData;
            other.mData = nullptr;
            other.clear();
        }
    }
    
    String::~String() {
        this->clear();
    }
    
    String& String::operator=(const String& rhs) {
        if (this == &rhs)
            return *this;
        this->clear();
        mSize = rhs.mSize;
        mCapacity = rhs.mCapacity;
        mMetric = rhs.mMetric;
        size_t len = mSize;
        if (mMetric == 1) {
            mData = StringPool::instance().get();
        } else if (mMetric == 2) {
            mData = new char[mCapacity];
        }
        memcpy(mData, rhs.mData, len);
        mData[len] = '\0';
        return *this;
    }
    
    String& String::operator=(String&& rhs) {
        if (this == &rhs)
            return *this;
        this->clear();
        mSize = rhs.mSize;
        mCapacity = rhs.mCapacity;
        mMetric = rhs.mMetric;
        size_t len = mSize;
        if (mMetric == 0) {
            memcpy(mData, rhs.mData, len);
            mData[len] = '\0';
        } else if (mMetric == 1 || mMetric == 2) {
            mData = rhs.mData;
            rhs.mData = nullptr;
            rhs.clear();
        }
        return *this;
    }
    
    String& String::operator+=(const String& rhs) {
        return this->append(rhs);
    }
    
    String String::operator+(const String& rhs) const {
        size_t len1 = mSize;
        if (len1 == 0)
            return rhs;
        
        size_t len2 = rhs.mSize;
        if (len2 == 0)
            return *this;
        
        String result;
        char metric = String::measure(len1 + len2);
        size_t capacity = _STRING_LITTLE_LENGTH;
        char* ptr = result.mData;
        if (metric == 1) {
            capacity = _STRING_MIDDLE_LENGTH;
            ptr = StringPool::instance().get();
        } else if (metric == 2) {
            capacity = String::predict(len1 + len2 + 1);
            ptr = new char[capacity];
        }
        memcpy(ptr, mData, len1);
        memcpy(ptr + len1, rhs.mData, len2);
        ptr[len1 + len2] = '\0';
        result.mData = ptr;
        result.mSize = len1 + len2;
        result.mCapacity = capacity;
        result.mMetric = metric;
        return result;
    }
    
    bool String::operator>(const String& rhs) const {
        return std::strcmp(mData, rhs.mData) > 0;
    }
    
    bool String::operator<(const String& rhs) const {
        return std::strcmp(mData, rhs.mData) < 0;
    }
    
    bool String::operator>=(const String& rhs) const {
        return std::strcmp(mData, rhs.mData) >= 0;
    }
    
    bool String::operator<=(const String& rhs) const {
        return std::strcmp(mData, rhs.mData) <= 0;
    }
    
    bool String::operator==(const String& rhs) const {
        return std::strcmp(mData, rhs.mData) == 0;
    }
    
    bool String::operator!=(const String& rhs) const {
        return std::strcmp(mData, rhs.mData) != 0;
    }
    
    const char* String::operator*() const {
        return this->cstr();
    }
    
    String& String::clear() {
        if (mData != nullptr) {
            if (mMetric == 1) {
                StringPool::instance().put(mData);
            } else if (mMetric == 2) {
                delete[]mData;
            }
            mData = nullptr;
        }
        
        mData = mValue;
        mSize = 0;
        mCapacity = _STRING_LITTLE_LENGTH;
        mMetric = 0;
        mValue[0] = '\0';
        return *this;
    }
    
    String& String::append(const String& str) {
        if (this == &str || str.isEmpty())
            return *this;
        
        size_t len1 = mSize;
        size_t len2 = str.mSize;
        
        size_t capacity = mCapacity;
        char metric = String::measure(len1 + len2);
        if (metric == mMetric) {
            if (metric == 0 || metric == 1) {
                memcpy(mData + len1, str.mData, len2);
                mData[len1 + len2] = '\0';
            } else if (metric == 2) {
                if (capacity >= len1 + len2 + 1) {
                    memcpy(mData + len1, str.mData, len2);
                    mData[len1 + len2] = '\0';
                } else {
                    capacity = String::predict(len1 + len2 + 1);
                    char* ptr = new char[capacity];
                    
                    memcpy(ptr, mData, len1);
                    memcpy(ptr + len1, str.mData, len2);
                    ptr[len1 + len2] = '\0';
                    
                    delete[]mData;
                    mData = ptr;
                }
            }
        } else {
            if (metric == 1) {
                capacity = _STRING_MIDDLE_LENGTH;
                char* ptr = StringPool::instance().get();
                
                memcpy(ptr, mData, len1);
                memcpy(ptr + len1, str.mData, len2);
                ptr[len1 + len2] = '\0';
                
                mData = ptr;
            } else if (metric == 2) {
                capacity = String::predict(len1 + len2 + 1);
                char* ptr = new char[capacity];
                
                memcpy(ptr, mData, len1);
                memcpy(ptr + len1, str.mData, len2);
                ptr[len1 + len2] = '\0';
                
                if (mMetric == 1) {
                    StringPool::instance().put(mData);
                }
                mData = ptr;
            }
        }
        mSize = len1 + len2;
        mCapacity = capacity;
        mMetric = metric;
        return *this;
    }
    
    String& String::insert(size_t pos, const String& str) {
        if (this == &str || str.isEmpty())
            return *this;
        
        if (pos > mSize) pos = mSize;
        
        size_t len1 = mSize;
        size_t len2 = str.mSize;
        
        size_t capacity = mCapacity;
        char metric = String::measure(len1 + len2);
        if (metric == mMetric) {
            if (metric == 0 || metric == 1) {
                memcpy(mData + pos + len2, mData + pos, len1 - pos);
                memcpy(mData + pos, str.mData, len2);
                mData[len1 + len2] = '\0';
            } else if (metric == 2) {
                if (capacity >= len1 + len2 + 1) {
                    memcpy(mData + pos + len2, mData + pos, len1 - pos);
                    memcpy(mData + pos, str.mData, len2);
                    mData[len1 + len2] = '\0';
                } else {
                    capacity = String::predict(len1 + len2 + 1);
                    char* ptr = new char[capacity];
                    
                    memcpy(ptr, mData, pos);
                    memcpy(ptr + pos, str.mData, len2);
                    memcpy(ptr + pos + len2, mData + pos, len1 - pos);
                    ptr[len1 + len2] = '\0';
                    
                    delete[]mData;
                    mData = ptr;
                }
            }
        } else {
            if (metric == 1) {
                capacity = _STRING_MIDDLE_LENGTH;
                char* ptr = StringPool::instance().get();
                
                memcpy(ptr, mData, pos);
                memcpy(ptr + pos, str.mData, len2);
                memcpy(ptr + pos + len2, mData + pos, len1 - pos);
                ptr[len1 + len2] = '\0';
                
                mData = ptr;
            } else if (metric == 2) {
                capacity = String::predict(len1 + len2 + 1);
                char* ptr = new char[capacity];
                
                memcpy(ptr, mData, pos);
                memcpy(ptr + pos, str.mData, len2);
                memcpy(ptr + pos + len2, mData + pos, len1 - pos);
                ptr[len1 + len2] = '\0';
                
                if (mMetric == 1) {
                    StringPool::instance().put(mData);
                }
                mData = ptr;
            }
        }
        mSize = len1 + len2;
        mCapacity = capacity;
        mMetric = metric;
        return *this;
    }
    
    String& String::remove(size_t pos, size_t len) {
        if (pos > mSize) pos = mSize;
        if (pos + len > mSize) len = mSize - pos;
        if (len == 0)
            return *this;
        if (len == mSize)
            return this->clear();
        
        size_t nlen = mSize - len;
        char metric = String::measure(nlen);
        if (metric == mMetric) {
            memcpy(mData + pos, mData + pos + len, mSize - pos - len);
            mData[nlen] = '\0';
            mSize = nlen;
        } else {
            if (metric == 0) {
                char* ptr = mValue;
                memcpy(ptr, mData, pos);
                memcpy(ptr + pos, mData + pos + len, mSize - pos - len);
                ptr[nlen] = '\0';
                if (mMetric == 1) {
                    StringPool::instance().put(mData);
                } else if (mMetric == 2) {
                    delete[]mData;
                }
                mData = ptr;
                mSize = nlen;
                mCapacity = _STRING_LITTLE_LENGTH;
                mMetric = metric;
            } else if (metric == 1) {
                char* ptr = StringPool::instance().get();
                memcpy(ptr, mData, pos);
                memcpy(ptr + pos, mData + pos + len, mSize - pos - len);
                ptr[nlen] = '\0';
                
                delete[]mData;
                
                mData = ptr;
                mSize = nlen;
                mCapacity = _STRING_MIDDLE_LENGTH;
                mMetric = metric;
            }
        }
        return *this;
    }
    
    size_t String::length() const {
        return mSize;
    }
    
    const char* String::cstr() const {
        return mData;
    }
    
    bool String::isEmpty() const {
        return mData[0] == '\0';
    }
    
    char String::at(size_t index) const {
        if (index >= mSize)
            return '\0';
        return mData[index];
    }
    
    size_t String::find(char chr, size_t pos) const {
        const char* ptr = ::strchr(mData + pos, chr);
        if (ptr == nullptr)
            return npos;
        return ptr - mData;
    }
    
    size_t String::find(const String& str, size_t pos) const {
        const char* ptr = ::strstr(mData + pos, str.mData);
        if (ptr == nullptr)
            return npos;
        return ptr - mData;
    }
    
    size_t String::rfind(char chr) const {
        const char* ptr = ::strrchr(mData, chr);
        if (ptr == nullptr)
            return npos;
        return ptr - mData;
    }
    
    size_t String::rfind(const String& str) const {
        const String& rs = this->reverse();
        const String& rf = str.reverse();
        
        const char* ptr = ::strstr(rs.mData, rf.mData);
        if (ptr == nullptr)
            return npos;
        size_t rp = ptr - rs.mData + rf.length();
        return this->length() - rp;
    }
    
    bool String::contains(const String& str) const {
        char* ptr = ::strstr(mData, str.mData);
        return ptr != nullptr;
    }
    
    bool String::containsOne(const StringVector& strs) const {
        auto iter = strs.begin();
        while (iter != strs.end()) {
            if (this->contains(*iter))
                return true;
            ++iter;
        }
        return false;
    }
    
    bool String::containsAll(const StringVector& strs) const {
        auto iter = strs.begin();
        while (iter != strs.end()) {
            if (this->contains(*iter))
                return false;
            ++iter;
        }
        return true;
    }
    
    bool String::startsWith(const String& str) const {
        size_t len1 = this->length();
        size_t len2 = str.length();
        if (len1 < len2 || len2 == 0)
            return false;
        return memcmp(mData, str.mData, len2) == 0;
    }
    
    bool String::startsWith(const StringVector& strs) const {
        auto iter = strs.begin();
        while (iter != strs.end()) {
            if (this->startsWith(*iter))
                return true;
            ++iter;
        }
        return false;
    }
    
    bool String::endsWith(const String& str) const {
        size_t len1 = this->length();
        size_t len2 = str.length();
        if (len1 < len2 || len2 == 0)
            return false;
        return memcmp(mData + len1 - len2, str.mData, len2) == 0;
    }
    
    bool String::endsWith(const StringVector& strs) const {
        auto iter = strs.begin();
        while (iter != strs.end()) {
            if (this->endsWith(*iter))
                return true;
            ++iter;
        }
        return false;
    }
    
    String String::toUpper() const {
        String result(*this);
        char* ptr = result.mData;
        while (*ptr != '\0') {
            *ptr = (char) toupper(*ptr);
            ptr++;
        }
        return result;
    }
    
    String String::toLower() const {
        String result(*this);
        char* ptr = result.mData;
        while (*ptr != '\0') {
            *ptr = (char) tolower(*ptr);
            ptr++;
        }
        return result;
    }
    
    String String::reverse() const {
        if (this->isEmpty())
            return "";
        String result(*this);
        char* src = this->mData;
        char* dst = result.mData + result.length() - 1;
        while (*src != '\0') {
            *dst-- = *src++;
        }
        return result;
    }
    
    String String::substr(size_t pos, size_t len) const {
        return String(mData + pos, len);
    }
    
    String String::left(size_t len) const {
        return this->substr(0, len);
    }
    
    String String::right(size_t len) const {
        size_t slen = this->length();
        if (slen < len)
            return mData;
        return this->substr(slen - len, len);
    }
    
    String String::replace(const String& str1, const String& str2) const {
        String result(*this);
        size_t index = result.find(str1);
        while (index != npos) {
            result = result.remove(index, str1.length()).insert(index, str2);
            index = result.find(str1, index + str2.length());
        }
        return result;
    }
    
    String String::replace(const StringMap& nameValues) const {
        String result(*this);
        auto iter = nameValues.begin();
        while (iter != nameValues.end()) {
            result = result.replace(iter->first, iter->second);
            ++iter;
        }
        return result;
    }
    
    String String::trim(bool left, bool right) const {
        if (this->isEmpty())
            return "";
        
        const char* beg = mData;
        const char* end = mData + this->length();
        if (left) {
            while (isspace(*beg))beg++;
        }
        if (right) {
            while (isspace(*(end-1))) end--;
        }
        return this->substr(beg - mData, end - beg);
    }
    
    StringVector String::split(const String& delim) const {
        StringVector result;
        size_t len = this->length();
        size_t beg = 0;
        while (beg < len) {
            size_t end = this->find(delim, beg);
            if (end == -1) {
                result.push_back(this->substr(beg));
                break;
            }
            
            if (end > beg) {
                result.push_back(this->substr(beg, end - beg));
            }
            beg = end + delim.length();
        }
        return result;
    }
    
    /*
    ============================================================================================
    ==== StringValue -- type casting class
    ============================================================================================
    */
    const StringValue StringValue::empty("");
    const StringValue StringValue::zero("0");
    const StringValue StringValue::one("1");
    const StringValue StringValue::trueValue("true");
    const StringValue StringValue::falseValue("false");
    
    StringValue::StringValue(const StringValue& var)
        : mValue(var.mValue) {
    }
    
    StringValue::StringValue(const String& value)
        : mValue(value) {
    }
    
    StringValue::StringValue(const char* value)
        : mValue(value) {
    }
    
    StringValue::StringValue(i8_t value)
        : mValue(value) {
    }
    
    StringValue::StringValue(u8_t value)
        : mValue(value) {
    }
    
    StringValue::StringValue(i16_t value)
        : mValue(String::valueToString(value)) {
    }
    
    StringValue::StringValue(u16_t value)
        : mValue(String::valueToString(value)) {
    }
    
    StringValue::StringValue(i32_t value)
        : mValue(String::valueToString(value)) {
    }
    
    StringValue::StringValue(u32_t value)
        : mValue(String::valueToString(value)) {
    }
    
    StringValue::StringValue(i64_t value)
        : mValue(String::valueToString(value)) {
    }
    
    StringValue::StringValue(u64_t value)
        : mValue(String::valueToString(value)) {
    }
    
    StringValue::StringValue(f32_t value)
        : mValue(String::valueToString(value)) {
    }
    
    StringValue::StringValue(f64_t value)
        : mValue(String::valueToString(value)) {
    }
    
    StringValue::StringValue(bool value)
        : mValue(String::valueToString(value)) {
    }
    
    StringValue& StringValue::operator=(const StringValue& var) {
        mValue = var.mValue;
        return *this;
    }
    
    StringValue& StringValue::operator=(const String& value) {
        mValue = value;
        return *this;
    }
    
    StringValue& StringValue::operator=(const char* value) {
        mValue = value;
        return *this;
    }
    
    StringValue& StringValue::operator=(i8_t value) {
        mValue = String::valueToString(value);
        return *this;
    }
    
    StringValue& StringValue::operator=(u8_t value) {
        mValue = String::valueToString(value);
        return *this;
    }
    
    StringValue& StringValue::operator=(i16_t value) {
        mValue = String::valueToString(value);
        return *this;
    }
    
    StringValue& StringValue::operator=(u16_t value) {
        mValue = String::valueToString(value);
        return *this;
    }
    
    StringValue& StringValue::operator=(i32_t value) {
        mValue = String::valueToString(value);
        return *this;
    }
    
    StringValue& StringValue::operator=(u32_t value) {
        mValue = String::valueToString(value);
        return *this;
    }
    
    StringValue& StringValue::operator=(i64_t value) {
        mValue = String::valueToString(value);
        return *this;
    }
    
    StringValue& StringValue::operator=(u64_t value) {
        mValue = String::valueToString(value);
        return *this;
    }
    
    StringValue& StringValue::operator=(f32_t value) {
        mValue = String::valueToString(value);
        return *this;
    }
    
    StringValue& StringValue::operator=(f64_t value) {
        mValue = String::valueToString(value);
        return *this;
    }
    
    StringValue& StringValue::operator=(bool value) {
        mValue = String::valueToString(value);
        return *this;
    }
    
    StringValue::operator const String&() const {
        return mValue;
    }
    
    StringValue::operator i8_t() const {
        return String::stringToValue<i8_t>(mValue);
    }
    
    StringValue::operator u8_t() const {
        return String::stringToValue<i8_t>(mValue);
    }
    
    StringValue::operator i16_t() const {
        return String::stringToValue<i16_t>(mValue);
    }
    
    StringValue::operator u16_t() const {
        return String::stringToValue<u16_t>(mValue);
    }
    
    StringValue::operator i32_t() const {
        return String::stringToValue<i32_t>(mValue);
    }
    
    StringValue::operator u32_t() const {
        return String::stringToValue<u32_t>(mValue);
    }
    
    StringValue::operator i64_t() const {
        return String::stringToValue<i64_t>(mValue);
    }
    
    StringValue::operator u64_t() const {
        return String::stringToValue<u64_t>(mValue);
    }
    
    StringValue::operator f32_t() const {
        return String::stringToValue<f32_t>(mValue);
    }
    
    StringValue::operator f64_t() const {
        return String::stringToValue<f64_t>(mValue);
    }
    
    StringValue::operator bool() const {
        return String::stringToValue<bool>(mValue);
    }
    
    bool StringValue::operator==(const StringValue& rhs) const {
        return mValue == rhs.mValue;
    }
    
    bool StringValue::operator!=(const StringValue& rhs) const {
        return mValue != rhs.mValue;
    }
    
    bool StringValue::operator==(const String& rhs) const {
        return mValue == rhs;
    }
    
    bool StringValue::operator!=(const String& rhs) const {
        return mValue != rhs;
    }
    
}
