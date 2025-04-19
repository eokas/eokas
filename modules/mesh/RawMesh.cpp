#include "./RawMesh.h"
#include <cassert>

namespace eokas
{
    void RawMesh::clear()
    {
        this->vertices.clear();
        this->corners.clear();
        this->attributes.clear();
        this->triangles.clear();
        this->sections.clear();
    }
    
    VertexID RawMesh::addVertex(const Vertex& vertex)
    {
        this->vertices.push_back(vertex);
        return this->vertices.size() - 1;
    }
    
    std::vector<VertexID> RawMesh::addVertices(const std::vector<Vertex>& vertexList)
    {
        std::vector<VertexID> result;
        for(auto& vertex : vertexList)
        {
            VertexID id = this->addVertex(vertex);
            result.push_back(id);
        }
        return result;
    }
    
    void RawMesh::setVertex(const VertexID& vertexId, const Vertex& vertex)
    {
        if(vertexId.value() < this->vertices.size())
        {
            this->vertices.at(vertexId.value()) = vertex;
        }
    }
    
    u32_t RawMesh::getVertexCount() const
    {
        return (u32_t)this->vertices.size();
    }
    
    const RawMesh::Vertex& RawMesh::getVertex(const VertexID& vertexId) const
    {
        return this->vertices.at(vertexId.value());
    }
    
    CornerID RawMesh::addCorner(const VertexID& v)
    {
        Corner& corner = this->corners.emplace_back();
        corner.vertexId = v;

        for(Attribute& attr : this->attributes)
        {
            attr.ensure(this->corners.size());
        }
        
        return this->corners.size() - 1;
    }
    
    std::vector<CornerID> RawMesh::addCorners(const std::vector<VertexID>& vertexList)
    {
        std::vector<CornerID> result;
        for(auto& vertex : vertexList)
        {
            CornerID id = this->addCorner(vertex);
            result.push_back(id);
        }
        return result;
    }
    
    AttributeID RawMesh::addAttribute(const AttributeType& type, u32_t stride)
    {
        Attribute& attr = this->attributes.emplace_back();
        attr.type = type;
        attr.stride = stride;
        attr.ensure(this->corners.size());
        return this->attributes.size() - 1;
    }
    
    AttributeID RawMesh::findAttribute(const AttributeType& type)
    {
        for (size_t i = 0; i < attributes.size(); ++i) {
            if (attributes[i].type == type) {
                return i;
            }
        }
        return {};
    }
    
    RawMesh::Attribute& RawMesh::getAttribute(const AttributeID& attributeId)
    {
        return this->attributes.at(attributeId.value());
    }
    
    TriangleID RawMesh::addTriangle(const Triangle& tri)
    {
        Triangle& triangle = this->triangles.emplace_back();
        triangle.c0 = tri.c0;
        triangle.c1 = tri.c1;
        triangle.c2 = tri.c2;
        return this->triangles.size() - 1;
    }
    
    TriangleID RawMesh::addTriangle(const CornerID& c0, const CornerID& c1, const CornerID& c2)
    {
        Triangle& triangle = this->triangles.emplace_back();
        triangle.c0 = c0;
        triangle.c1 = c1;
        triangle.c2 = c2;
        return this->triangles.size() - 1;
    }
    
    std::vector<TriangleID> RawMesh::addTriangleList(const std::vector<CornerID>& corners)
    {
        if(corners.size() /3 != 0)
            return {};
        
        std::vector<TriangleID> result;
        u32_t triangleCount = u32_t(corners.size() / 3);
        for(u32_t i = 0; i < triangleCount; i++)
        {
            u32_t base = i * 3;
            TriangleID tri = this->addTriangle(corners[base], corners[base + 1], corners[base + 2]);
            result.push_back(tri);
        }
        
        return result;
    }
    
    void RawMesh::delTriangle(const TriangleID& triangleId)
    {
        if(triangleId.value() < this->triangles.size())
        {
            this->triangles.erase(this->triangles.begin() + triangleId.value());
        }
    }
    
    u32_t RawMesh::getTriangleCount() const
    {
        return (u32_t)this->triangles.size();
    }
    
    const RawMesh::Triangle& RawMesh::getTriangle(const TriangleID& triangleId) const
    {
        return this->triangles.at(triangleId.value());
    }
    
    SectionID RawMesh::addSection()
    {
        this->sections.emplace_back();
        return this->sections.size() - 1;
    }
    
    void RawMesh::addSectionTriangle(const SectionID& sectionId, const TriangleID& triangleId)
    {
        for(auto& section : this->sections)
        {
            section.remove(triangleId);
        }
        
        this->sections.at(sectionId.value()).triangles.push_back(triangleId);
    }
    
    void RawMesh::addSectionTriangles(const SectionID& sectionId, const std::vector<TriangleID>& triangleList)
    {
        for(auto& section : this->sections)
        {
            for(auto& triangleId : triangleList)
            {
                section.remove(triangleId);
            }
        }
        
        auto& section = this->sections.at(sectionId.value());
        for(auto& triangleId : triangleList)
        {
            section.triangles.push_back(triangleId);
        }
    }
    
    void RawMesh::delSectionTriangle(const SectionID& sectionId, const TriangleID& triangleId)
    {
        Section& section = this->sections.at(sectionId.value());
        section.remove(triangleId);
    }
    
    const std::vector<TriangleID>& RawMesh::getSectionTriangles(const SectionID& sectionId) const
    {
        return this->sections.at(sectionId.value()).triangles;
    }
    
    void RawMesh::clearSectionTriangles(const SectionID& sectionId)
    {
        this->sections.at(sectionId.value()).triangles.clear();
    }
    
    void RawMesh::delSection(const SectionID& sectionId)
    {
        this->sections.erase(this->sections.begin() + sectionId.value());
    }
}