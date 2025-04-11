#ifndef _EOKAS_HALFEDGEMESH_H_
#define _EOKAS_HALFEDGEMESH_H_

#include "base/main.h"

namespace eokas
{
    struct HalfEdgeMesh
    {
        struct Vertex
        {
            Vector3 position;
            struct HalfEdge* edge;
        };
        
        struct HalfEdge
        {
            Vertex* v0;
            Vertex* v1;
            HalfEdge* twin;
            HalfEdge* prev;
            HalfEdge* next;
        };
        
        std::vector<HalfEdge> edges;
    };
}

#endif//_EOKAS_HALFEDGEMESH_H_