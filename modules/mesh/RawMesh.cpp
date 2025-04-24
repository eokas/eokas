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
        if(vertexId < this->vertices.size())
        {
            this->vertices.at(vertexId) = vertex;
        }
    }
    
    u32_t RawMesh::getVertexCount() const
    {
        return (u32_t)this->vertices.size();
    }
    
    const RawMesh::Vertex& RawMesh::getVertex(const VertexID& vertexId) const
    {
        return this->vertices.at(vertexId);
    }
    
    const RawMesh::Vertex& RawMesh::getVertex(const CornerID& cornerId) const
    {
        const Corner& corner = this->corners.at(cornerId);
        return this->vertices.at(corner.vertexId);
    }
    
    CornerID RawMesh::addCorner(const VertexID& v)
    {
        Corner& corner = this->corners.emplace_back();
        corner.vertexId = v;
        
        auto visitor = [size = this->corners.size()](auto& attribute)
        {
            attribute.ensure(size);
        };
        for(AttributeVariant& attributeVariant : this->attributes)
        {
            std::visit(visitor, attributeVariant);
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
    
    AttributeID RawMesh::findAttribute(const AttributeUsage& usage)
    {
        for (size_t id = 0; id < attributes.size(); ++id)
        {
            bool match = std::visit([&usage](auto&& arg) -> bool
            {
                return arg.usage() == usage;
            }, attributes[id]);
            
            if (match)
            {
                return id;
            }
        }
        
        return {};
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
        if(triangleId < this->triangles.size())
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
        return this->triangles.at(triangleId);
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
        
        this->sections.at(sectionId).triangles.push_back(triangleId);
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
        Section& section = this->sections.at(sectionId);
        section.remove(triangleId);
    }
    
    const std::vector<TriangleID>& RawMesh::getSectionTriangles(const SectionID& sectionId) const
    {
        return this->sections.at(sectionId).triangles;
    }
    
    void RawMesh::clearSectionTriangles(const SectionID& sectionId)
    {
        this->sections.at(sectionId).triangles.clear();
    }
    
    void RawMesh::delSection(const SectionID& sectionId)
    {
        this->sections.erase(this->sections.begin() + sectionId.value());
    }
    
    void RawMesh::computeNormals(NormalMode normalMode)
    {
        if (this->vertices.empty() || this->triangles.empty())
        {
            return;
        }
        
        // Find or Add normal attribute
        AttributeID normalAttrId = this->findAttribute(AttributeUsage::Normal);
        if (!normalAttrId.isValid())
        {
            normalAttrId = this->addAttribute<Vector3>(AttributeUsage::Normal);
        }
        Attribute<Vector3>& normalAttr = this->getAttribute<Vector3>(normalAttrId);
        
        // Ensure normals' space
        normalAttr.ensure(this->corners.size());
        
        if(normalMode == NormalMode::Flat)
        {
            // Compute face normal for each triangle.
            for (const Triangle& tri: triangles)
            {
                const Vertex& v0 = this->getVertex(tri.c0);
                const Vertex& v1 = this->getVertex(tri.c1);
                const Vertex& v2 = this->getVertex(tri.c2);
                
                Vector3 edge1 = v1.pos - v0.pos;
                Vector3 edge2 = v2.pos - v0.pos;
                Vector3 faceNormal = Vector3::cross(edge1, edge2);
                Vector3 normal = faceNormal.sqrmagnitude() > 0.0f ? faceNormal.normalized() : Vector3(0, 0, 1);
                
                normalAttr.set(tri.c0, normal);
                normalAttr.set(tri.c1, normal);
                normalAttr.set(tri.c2, normal);
            }
        }
        else if(normalMode == NormalMode::Smooth)
        {
            std::vector<Vector3> vertexNormals(vertices.size(), Vector3(0, 0, 1));
            std::vector<float> vertexWeights(vertices.size(), 0.0f);
            
            // Compute face normal for each triangle.
            for (const Triangle& tri: triangles)
            {
                const Vertex& v0 = this->getVertex(tri.c0);
                const Vertex& v1 = this->getVertex(tri.c1);
                const Vertex& v2 = this->getVertex(tri.c2);
                
                Vector3 edge1 = v1.pos - v0.pos;
                Vector3 edge2 = v2.pos - v0.pos;
                Vector3 faceNormal = Vector3::cross(edge1, edge2);
                
                // Triangle Area for weight.
                float area = faceNormal.magnitude();
                if (area > 0.0f)
                {
                    faceNormal *= (1.0f / area); // Normalize
                    area *= 0.5f; // Actual area is 0.5 * cross-product.
                    
                    // Add normal by area-weight
                    VertexID vid0 = corners[tri.c0].vertexId;
                    VertexID vid1 = corners[tri.c1].vertexId;
                    VertexID vid2 = corners[tri.c2].vertexId;
                    
                    vertexNormals[vid0] += faceNormal * area;
                    vertexNormals[vid1] += faceNormal * area;
                    vertexNormals[vid2] += faceNormal * area;
                    
                    vertexWeights[vid0] += area;
                    vertexWeights[vid1] += area;
                    vertexWeights[vid2] += area;
                }
            }
            
            // Normalized normal and set it to attribute
            for (size_t i = 0; i < corners.size(); ++i)
            {
                const Corner& corner = corners[i];
                VertexID vid = corner.vertexId;
                
                Vector3 normal(0, 0, 1);
                if (vertexWeights[vid] > 0.0f)
                {
                    normal = vertexNormals[vid];
                    normal.normalize();
                }
                
                normalAttr.set(CornerID(static_cast<u32_t>(i)), normal);
            }
        }
    }
}