#pragma once
#include <vector>
#include <string>

struct Vector2 { float x, y; };
struct Vector3 { float x, y, z; };
struct Vector4 { float x, y, z, w; };

struct Vertex
{
    Vector3 position;
    Vector3 normal;
    Vector4 color;
    Vector2 uv;
};

struct MeshInfo
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

struct MeshFactory
{
    static void createQuad(MeshInfo& mesh, float width, float height);
    static void createBox(MeshInfo& mesh, float width, float height, float depth);
    static void createSphere(MeshInfo& mesh, float radius, int longitudeSegments, int latitudeSegments);
};

struct Utilities
{
    static void createImage(std::vector<uint8_t>& image, uint32_t textureWidth, uint32_t textureHeight);
    static std::string readTextFile(const std::string& fileName);
};
