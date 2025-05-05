#ifndef _EOKAS_MESH_RAWMESH_H_
#define _EOKAS_MESH_RAWMESH_H_

#include "./Header.h"
#include <variant>

namespace eokas
{
    class RawMesh
    {
    public:
        static const u32_t VERTEX_BONE_WEIGHT_COUNT = 4;
        
        struct Vertex
        {
            Vector3 pos;
        };
        
        struct Corner
        {
            VertexID vertexId;
        };
        
        enum class AttributeUsage
        {
            Color, UV, Normal, Tangent, Binormal,
        };
        
        template<typename T>
        struct Attribute
        {
            Attribute(const AttributeUsage& usage)
                : mUsage(usage)
                , mBuffer()
            { }
            
            const AttributeUsage& usage() const
            {
                return mUsage;
            }
            
            u32_t stride() const
            {
                return sizeof(T);
            }
            
            u32_t corners() const
            {
                return u32_t(mBuffer.size());
            }
            
            void ensure(size_t count)
            {
                if(mBuffer.size() < count)
                {
                    mBuffer.resize(count);
                }
            }
            
            void get(const CornerID& id, T& val)
            {
                if(id >= mBuffer.size())
                    return;
                val = mBuffer.at(id);
            }
            
            void set(const CornerID& id, const T& val)
            {
                if (id >= mBuffer.size())
                    return;
                mBuffer[id] = val;
            }
            
        private:
            AttributeUsage mUsage;
            std::vector<T> mBuffer;
        };
        
        using AttributeVariant = std::variant<
            Attribute<f32_t>,
            Attribute<Vector2>,
            Attribute<Vector3>,
            Attribute<Vector4>>;
        
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
        
        enum class NormalMode
        {
            Flat,
            Smooth,
        };
        
        struct Weight
        {
            BoneID bones[VERTEX_BONE_WEIGHT_COUNT]{};
            float weights[VERTEX_BONE_WEIGHT_COUNT]{};
            
            void normalize()
            {
                float sum = 0;
                for(float weight : weights)
                {
                    sum += weight;
                }
                if(sum > 0)
                {
                    for(float& weight : weights)
                    {
                        weight /= sum;
                    }
                }
            }
        };
        
        void clear();
        
        VertexID addVertex(const Vertex& vertex);
        std::vector<VertexID> addVertices(const std::vector<Vertex>& vertexList);
        void setVertex(const VertexID& vertexId, const Vertex& vertex);
        u32_t getVertexCount() const;
        const Vertex& getVertex(const VertexID& vertexId) const;
        const Vertex& getVertex(const CornerID& cornerId) const;
        
        void setWeight(const VertexID& vertexId, const Weight& weight);
        void setWeights(const std::vector<Weight>& weightList);
        const Weight& getWeight(const VertexID& vertexId);
        
        CornerID addCorner(const VertexID& v);
        std::vector<CornerID> addCorners(const std::vector<VertexID>& vertexList);
        const Corner& getCorner(const CornerID& cornerId) const;
        
        template<typename T>
        AttributeID addAttribute(const AttributeUsage& usage)
        {
            AttributeVariant& attributeVariant = this->attributes.emplace_back(Attribute<T>(usage));
            Attribute<T>& attr = std::get<Attribute<T>>(attributeVariant);
            attr.ensure(this->corners.size());
            return this->attributes.size() - 1;
        }
        
        AttributeID findAttribute(const AttributeUsage& usage);
        
        template<typename T>
        Attribute<T>& getAttribute(const AttributeID& attributeId)
        {
            auto attribute = this->attributes.at(attributeId);
            return std::get<Attribute<T>>(attribute);
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
        
        void computeNormals(NormalMode normalMode);
        
    private:
        std::vector<Vertex> vertices;
        std::vector<Weight> weights;
        
        std::vector<Corner> corners;
        std::vector<AttributeVariant> attributes;
        std::vector<Triangle> triangles;
        std::vector<Section> sections;
    };
}

#endif//_EOKAS_MESH_RAWMESH_H_