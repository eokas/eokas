#ifndef _EOKAS_HALFEDGEMESH_H_
#define _EOKAS_HALFEDGEMESH_H_

#include "base/main.h"

namespace eokas
{
    struct HalfEdgeMesh
    {
        struct Vertex;
        struct Edge;
        struct Polygon;
        
        struct Vertex
        {
            Vector3 pos;
            Edge* edge;
            std::vector<Edge*> adjacentEdges;
        };
        
        struct Edge
        {
            Polygon* polygon;
            Vertex* vertex;
            Edge* twin;
            Edge* prev;
            Edge* next;
        };
        
        struct Polygon
        {
            Edge* edge;
        };
        
        std::vector<Vertex> vertices;
        std::vector<Edge> edges;
        std::vector<Polygon> polygons;
        
        Vertex* addVertex(const Vector3& p);
        Polygon* addTriangle(Vertex* v0, Vertex* v1, Vertex* v2);
        Polygon* addPolygon(const std::vector<Vertex*>& vertexList);
    };
}

#endif//_EOKAS_HALFEDGEMESH_H_