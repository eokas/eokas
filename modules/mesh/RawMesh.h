#ifndef _EOKAS_MESH_RAWMESH_H_
#define _EOKAS_MESH_RAWMESH_H_

#include "./header.h"

namespace eokas
{
    class RawMesh
    {
    public:
        struct Vertex
        {
            Vector3 pos;
        };
        
        struct Corner
        {
            VertexID vertexId;
        };
        
        enum class AttributeType
        {
            Color, UV, Normal, Tangent, Binormal,
        };
        
        struct Attribute
        {
            AttributeType type;
            u32_t stride;
            std::vector<u8_t> buffer;
            
            u32_t corners() const
            {
                return u32_t(this->buffer.size() / this->stride);
            }
            
            void ensure(size_t count)
            {
                if(this->corners() < count)
                {
                    this->buffer.resize(this->stride * this->corners());
                }
            }
            
            template<typename T>
            void get(const CornerID& id, T& val)
            {
                if(this->stride != sizeof(T))
                    return;
                const size_t count = this->buffer.size() / this->stride;
                if(id.value() >= count)
                    return;
                
                u8_t* ptr = this->buffer.data() + id.value() * stride;
                memcpy(&val, ptr, sizeof(val));
            }
            
            template<typename T>
            void set(const CornerID& id, const T& val)
            {
                if(this->stride != sizeof(T))
                    return;
                const size_t count = this->buffer.size() / this->stride;
                if (id.value() >= count)
                    return;
                u8_t* ptr = this->buffer.data() + id.value() * stride;
                memcpy(&ptr, &val, sizeof(val));
            }
        };
        
        template<typename T>
        struct AttributeRef
        {
            Attribute& attribute;
            
            AttributeRef(Attribute& attr) : attribute(attr) {}
            
            AttributeType& type() const
            {
                return this->attribute.type;
            }
            
            u32_t stride() const
            {
                return this->attribute.stride;
            }
            
            void get(const CornerID& id, T& val)
            {
                return this->attribute.get(id, val);
            }
            
            void set(const CornerID& id, const T& val)
            {
                this->attribute.set(id, val);
            }
        };
        
        struct Triangle
        {
            CornerID c0;
            CornerID c1;
            CornerID c2;
        };

        struct Section
        {
            std::vector<TriangleID> triangles;
            
            bool contains(const TriangleID& triangleId)
            {
                if(triangles.empty())
                    return false;
                auto iter = std::find(triangles.begin(), triangles.end(), triangleId);
                return iter != triangles.end();
            }
            
            void remove(const TriangleID& triangleId)
            {
                if(triangles.empty())
                    return;
                auto iter = std::find(triangles.begin(), triangles.end(), triangleId);
                if(iter != triangles.end())
                {
                    iter = triangles.erase(iter);
                }
            }
        };
        
        void clear();
        
        VertexID addVertex(const Vertex& vertex);
        std::vector<VertexID> addVertices(const std::vector<Vertex>& vertexList);
        void setVertex(const VertexID& vertexId, const Vertex& vertex);
        u32_t getVertexCount() const;
        const Vertex& getVertex(const VertexID& vertexId) const;
        
        CornerID addCorner(const VertexID& v);
        std::vector<CornerID> addCorners(const std::vector<VertexID>& vertexList);
        
        AttributeID addAttribute(const AttributeType& type, u32_t stride);
        AttributeID findAttribute(const AttributeType& type);
        Attribute& getAttribute(const AttributeID& attributeId);
        
        template<typename T>
        AttributeRef<T> getAttribute(const AttributeID& attributeId)
        {
            return this->getAttribute(attributeId);
        }
        
        TriangleID addTriangle(const Triangle& triangle);
        TriangleID addTriangle(const CornerID& c0, const CornerID& c1, const CornerID& c2);
        std::vector<TriangleID> addTriangleList(const std::vector<CornerID>& corners);
        void delTriangle(const TriangleID& triangleId);
        u32_t getTriangleCount() const;
        const Triangle& getTriangle(const TriangleID& triangleId) const;
        
        SectionID addSection();
        void addSectionTriangle(const SectionID& sectionId, const TriangleID& triangleId);
        void addSectionTriangles(const SectionID& sectionId, const std::vector<TriangleID>& triangleList);
        void delSectionTriangle(const SectionID& sectionId, const TriangleID& triangleId);
        const std::vector<TriangleID>& getSectionTriangles(const SectionID& sectionId) const;
        void clearSectionTriangles(const SectionID& sectionId);
        void delSection(const SectionID& sectionId);
        
    private:
        std::vector<Vertex> vertices;
        std::vector<Corner> corners;
        std::vector<Attribute> attributes;
        std::vector<Triangle> triangles;
        std::vector<Section> sections;
    };
}

#endif//_EOKAS_MESH_RAWMESH_H_