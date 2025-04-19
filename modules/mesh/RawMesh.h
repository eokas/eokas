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
            f32_t x;
            f32_t y;
            f32_t z;
        };
        
        struct Corner
        {
            VertexID vertexId;
        };
        
        struct Attribute
        {
            u32_t stride;
            std::vector<u8_t> buffer;
            
            void alloc(const CornerID& id);
            
            void get(const CornerID& id, Vector2& val);
            void get(const CornerID& id, Vector3& val);
            void get(const CornerID& id, Color& val);
            
            void set(const CornerID& id, const Vector2& val);
            void set(const CornerID& id, const Vector3& val);
            void set(const CornerID& id, const Color& val);
        };
        
        struct Triangle
        {
            CornerID c0;
            CornerID c1;
            CornerID c2;
        };
        
        struct Polygon
        {
            std::vector<CornerID> corners;
        };
        
        struct Section
        {
            std::vector<PolygonID> polygons;
        };
        
        VertexID addVertex(const Vertex& v);
        CornerID addCorner(const VertexID& v);
        AttributeID addAttribute(u32_t stride);
        Attribute& getAttribute(const AttributeID& attributeId);

    private:
        std::vector<Vertex> vertices;
        std::vector<Corner> corners;
        std::vector<Attribute> attributes;
        std::vector<Triangle> triangles;
        std::vector<Polygon> polygons;
        std::vector<Section> sections;
    };
}

#endif//_EOKAS_MESH_RAWMESH_H_