#ifndef _EOKAS_GEOMESH_H_
#define _EOKAS_GEOMESH_H_

#include "base/main.h"

namespace eokas
{
    struct Vertex
    {
        Vector3 position;
        Vector3 normal;
        Vector4 color;
        Vector2 uv;
    };
    
    struct GeoMesh
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
    };
    
    struct GeoMeshFactory
    {
        static void createQuad(GeoMesh& mesh, float width, float height);
        static void createBox(GeoMesh& mesh, float width, float height, float depth);
        static void createSphere(GeoMesh& mesh, float radius, int longitudeSegments, int latitudeSegments);
        static void createCylinder(GeoMesh& mesh, float radius, float height, int longitudeSegments, int latitudeSegments);
    };
}

#endif//_EOKAS_GEOMESH_H_