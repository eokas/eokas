#ifndef _EOKAS_MESH_HEADER_H_
#define _EOKAS_MESH_HEADER_H_

#include "base/main.h"

namespace eokas
{
    class ElementID
    {
    public:
        ElementID(u32_t value = -1)
            : mValue(value)
        {}
        
        ElementID(const ElementID& other)
            : mValue(other.mValue)
        {}
        
        operator u32_t() const
        {
            return mValue;
        }
        
        bool operator==(const ElementID& rhs) const
        {
            return mValue == rhs.mValue;
        }
        
        bool operator==(size_t rhs) const
        {
            return mValue == rhs;
        }
        
        bool operator!=(const ElementID& rhs) const
        {
            return mValue != rhs.mValue;
        }
        
        bool operator!=(size_t rhs) const
        {
            return mValue != rhs;
        }
        
        bool operator<(const ElementID& rhs) const
        {
            return mValue < rhs.mValue;
        }
        
        bool operator<(size_t rhs) const
        {
            return mValue < rhs;
        }
        
        bool operator<=(const ElementID& rhs) const
        {
            return mValue <= rhs.mValue;
        }
        
        bool operator<=(size_t rhs) const
        {
            return mValue <= rhs;
        }
        
        bool operator>(const ElementID& rhs) const
        {
            return mValue > rhs.mValue;
        }
        
        bool operator>(size_t rhs) const
        {
            return mValue > rhs;
        }
        
        bool operator>=(const ElementID& rhs) const
        {
            return mValue >= rhs.mValue;
        }
        
        bool operator>=(size_t rhs) const
        {
            return mValue >= rhs;
        }
        
        u32_t value() const
        {
            return mValue;
        }
        
        bool isValid() const
        {
            return mValue != -1;
        }
        
    private:
        u32_t mValue;
    };
    
#define DefineElementID(type) \
    struct type : public ElementID \
    { \
        type(u32_t val = -1) : ElementID(val) {} \
    }
    
    DefineElementID(VertexID);
    DefineElementID(CornerID);
    DefineElementID(AttributeID);
    DefineElementID(TriangleID);
    DefineElementID(PolygonID);
    DefineElementID(SectionID);
    DefineElementID(BoneID);
}

#endif//_EOKAS_MESH_HEADER_H_