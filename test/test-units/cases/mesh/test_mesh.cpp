#include "Unit.h"
#include "mesh/GeoMesh.h"
#include "mesh/MeshFactory.h"
#include "mesh/RawMesh.h"
#include "mesh/Skeleton.h"

using namespace eokas;

EOKAS_TEST_CASE(mesh) {
    ElementID invalid;
    EOKAS_EXPECT(!invalid.isValid());
    ElementID zero(0);
    EOKAS_EXPECT(zero.isValid());
    EOKAS_EXPECT(zero < ElementID(1));

    RawMesh raw;
    RawMesh::Vertex a;
    RawMesh::Vertex b;
    RawMesh::Vertex c;
    a.pos = Vector3(0, 0, 0);
    b.pos = Vector3(1, 0, 0);
    c.pos = Vector3(0, 1, 0);
    VertexID v0 = raw.addVertex(a);
    VertexID v1 = raw.addVertex(b);
    VertexID v2 = raw.addVertex(c);
    EOKAS_EXPECT(raw.getVertexCount() == 3);
    EOKAS_EXPECT(raw.getVertex(v1).pos == Vector3(1, 0, 0));

    RawMesh::Weight weight;
    weight.weights[0] = 1.0f;
    weight.weights[1] = 1.0f;
    weight.normalize();
    EOKAS_EXPECT(_FloatEqual(weight.weights[0], 0.5f));
    raw.setWeight(v0, weight);
    EOKAS_EXPECT(_FloatEqual(raw.getWeight(v0).weights[0], 0.5f));

    std::vector<VertexID> cornerVertices;
    cornerVertices.push_back(v0);
    cornerVertices.push_back(v1);
    cornerVertices.push_back(v2);
    std::vector<CornerID> corners = raw.addCorners(cornerVertices);
    TriangleID triangle = raw.addTriangle(corners[0], corners[1], corners[2]);
    EOKAS_EXPECT(raw.getTriangleCount() == 1);
    EOKAS_EXPECT(raw.getTriangle(triangle).c0 == corners[0]);

    AttributeID uv = raw.addAttribute<Vector2>(RawMesh::AttributeUsage::UV);
    EOKAS_EXPECT(raw.findAttribute(RawMesh::AttributeUsage::UV) == uv);
    SectionID section = raw.addSection();
    raw.addSectionTriangle(section, triangle);
    EOKAS_EXPECT(raw.getSectionTriangles(section).size() == 1);
    EOKAS_EXPECT(raw.getSectionTriangles(section)[0] == triangle);

    GeoMesh quad;
    GeoMeshFactory::createQuad(quad, 2.0f, 4.0f);
    EOKAS_EXPECT(quad.vertices.size() == 4);
    EOKAS_EXPECT(quad.indices.size() == 6);
    EOKAS_EXPECT(_FloatEqual(quad.vertices[0].position.x, -1.0f));
    EOKAS_EXPECT(_FloatEqual(quad.vertices[0].position.y, 2.0f));

    GeoMesh box;
    GeoMeshFactory::createBox(box, 1.0f, 1.0f, 1.0f);
    EOKAS_EXPECT(box.vertices.size() >= 8);
    EOKAS_EXPECT(box.indices.size() % 3 == 0);

    GeoMesh sphere;
    GeoMeshFactory::createSphere(sphere, 1.0f, 8, 4);
    EOKAS_EXPECT(!sphere.vertices.empty());
    EOKAS_EXPECT(sphere.indices.size() % 3 == 0);

    RawMesh factoryQuad;
    MeshFactory::createQuad(factoryQuad, 2.0f, 2.0f);
    EOKAS_EXPECT(factoryQuad.getVertexCount() == 4);
    EOKAS_EXPECT(factoryQuad.getTriangleCount() == 2);

    Skeleton skeleton;
    BoneID root = skeleton.addBone("root", BoneID());
    BoneID child = skeleton.addBone("child", root);
    EOKAS_EXPECT(skeleton.getBoneCount() == 2);
    EOKAS_EXPECT(skeleton.findBone("child") == child);
    EOKAS_EXPECT(skeleton.getBone(child).parent == root);

    Skeleton::Pose left;
    Skeleton::Pose right;
    left.transforms.push_back({root, Vector3(0, 0, 0), Quaternion::IDENTITY, Vector3(1, 1, 1)});
    right.transforms.push_back({root, Vector3(2, 0, 0), Quaternion::IDENTITY, Vector3(3, 1, 1)});
    Skeleton::Pose mixed = Skeleton::Pose::blend(left, right, 0.5f);
    EOKAS_EXPECT(mixed.transforms.size() == 1);
    EOKAS_EXPECT(_FloatEqual(mixed.transforms[0].localPosition.x, 1.0f));
    EOKAS_EXPECT(_FloatEqual(mixed.transforms[0].localScale.x, 2.0f));
    return 0;
}
