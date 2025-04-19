#ifndef _EOKAS_MESH_MESHFACTORY_H_
#define _EOKAS_MESH_MESHFACTORY_H_

#include "./RawMesh.h"

namespace eokas
{
    struct MeshFactory
    {
        static void createBox(RawMesh& mesh, float width, float height, float depth);
    };
}

#endif//_EOKAS_MESH_MESHFACTORY_H_