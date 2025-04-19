#include "./RawMesh.h"

namespace eokas
{
    void RawMesh::Attribute::alloc(const CornerID& id)
    {
        const size_t corners = this->buffer.size() / this->stride;
        while(id.value() >= corners)
        {
            for(u32_t i = 0; i < this->stride; i++)
            {
                this->buffer.emplace_back(0);
            }
        }
    }
    
    void RawMesh::Attribute::get(const CornerID& id, Vector2& val)
    {
        u8_t* ptr = this->buffer.data() + id.value() * stride;
        memcpy(&val, ptr, sizeof(val));
    }
    
    void RawMesh::Attribute::get(const CornerID& id, Vector3& val)
    {
        u8_t* ptr = this->buffer.data() + id.value() * stride;
        memcpy(&val, ptr, sizeof(val));
    }
    
    void RawMesh::Attribute::get(const CornerID& id, Color& val)
    {
        u8_t* ptr = this->buffer.data() + id.value() * stride;
        memcpy(&val, ptr, sizeof(val));
    }
    
    void RawMesh::Attribute::set(const CornerID& id, const Vector2& val)
    {
        u8_t* ptr = this->buffer.data() + id.value() * stride;
        memcpy(&ptr, &val, sizeof(val));
    }
    
    void RawMesh::Attribute::set(const CornerID& id, const Vector3& val)
    {
        u8_t* ptr = this->buffer.data() + id.value() * stride;
        memcpy(&ptr, &val, sizeof(val));
    }
    
    void RawMesh::Attribute::set(const CornerID& id, const Color& val)
    {
        u8_t* ptr = this->buffer.data() + id.value() * stride;
        memcpy(&ptr, &val, sizeof(val));
    }
    
    VertexID RawMesh::addVertex(const Vertex& v)
    {
        this->vertices.push_back(v);
        return this->vertices.size() - 1;
    }
    
    CornerID RawMesh::addCorner(const VertexID& v)
    {
        Corner& corner = this->corners.emplace_back();
        corner.vertexId = v;
        
        CornerID cornerId = this->corners.size() - 1;
        
        for(Attribute& attr : this->attributes)
        {
            attr.alloc(cornerId);
        }
        
        return cornerId;
    }
    
    AttributeID RawMesh::addAttribute(u32_t stride)
    {
        Attribute& attr = this->attributes.emplace_back();
        attr.stride = stride;
        return this->attributes.size() - 1;
    }
    
    RawMesh::Attribute& RawMesh::getAttribute(const AttributeID& attributeId)
    {
        return this->attributes.at(attributeId.value());
    }
    
}