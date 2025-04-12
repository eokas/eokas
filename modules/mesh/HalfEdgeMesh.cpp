#include "./HalfEdgeMesh.h"

namespace eokas
{
    HalfEdgeMesh::Vertex* HalfEdgeMesh::addVertex(const eokas::Vector3& p)
    {
        auto& vertex = this->vertices.emplace_back();
        vertex.pos = p;
        return &vertex;
    }
    
    HalfEdgeMesh::Polygon* HalfEdgeMesh::addTriangle(Vertex* v0, Vertex* v1, Vertex* v2)
    {
        auto& f = this->polygons.emplace_back();
        
        auto& e0 = this->edges.emplace_back();
        auto& e1 = this->edges.emplace_back();
        auto& e2 = this->edges.emplace_back();
        
        e0.polygon = &f;
        e0.vertex = v0;
        e0.next = &e1;
        e0.prev = &e2;
        
        e1.polygon = &f;
        e1.vertex = v1;
        e1.next = &e2;
        e1.prev = &e0;
        
        e2.polygon = &f;
        e2.vertex = v2;
        e2.next = &e0;
        e2.prev = &e1;
        
        f.edge = &e0;
        
        return &f;
    }
    
    HalfEdgeMesh::Polygon* HalfEdgeMesh::addPolygon(const std::vector<Vertex*>& vertexList)
    {
        auto& f = this->polygons.emplace_back();
        
        std::vector<Edge*> edgeList;
        for(auto* v : vertexList)
        {
            auto& e = this->edges.emplace_back();
            e.polygon = &f;
            e.vertex = v;
            edgeList.push_back(&e);
        }
        
        for(size_t i = 0; i < edgeList.size(); i++)
        {
            size_t prev = (i - 1) % edgeList.size();
            size_t next = (i + 1) % edgeList.size();
            edgeList[i]->next = edgeList[next];
            edgeList[i]->prev = edgeList[prev];
        }
        
        f.edge = edgeList[0];
        
        return &f;
    }
}