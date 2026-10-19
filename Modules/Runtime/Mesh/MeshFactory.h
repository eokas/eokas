#ifndef _EOKAS_MESH_MESHFACTORY_H_
#define _EOKAS_MESH_MESHFACTORY_H_

#include "./RawMesh.h"

namespace eokas
{
    struct MeshFactory
    {
        static void createQuad(RawMesh& mesh, float width, float height);
        static void createBox(RawMesh& mesh, float width, float height, float depth);
        static void createSphere(RawMesh& mesh, float radius, int longitudeSegments, int latitudeSegments);
        static void createCylinder(RawMesh& mesh, float radius, float height, int longitudeSegments, int latitudeSegments);
    };
}

#endif//_EOKAS_MESH_MESHFACTORY_H_