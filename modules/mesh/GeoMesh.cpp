#include "./GeoMesh.h"

namespace eokas
{
#define M_PI 3.141592653f
    
    void GeoMeshFactory::createQuad(GeoMesh& mesh, float width, float height)
    {
        mesh.vertices.clear();
        mesh.indices.clear();
        
        // Half dimensions for centered quad
        float halfW = width * 0.5f;
        float halfH = height * 0.5f;
        
        // Define vertex positions (facing -Z)
        Vector3 positions[4] = {
            {-halfW,  halfH, 0.0f},
            { halfW,  halfH, 0.0f},
            { halfW, -halfH, 0.0f},
            {-halfW, -halfH, 0.0f}
        };
        
        // Normal facing -Z (towards screen)
        Vector3 normal = {0.0f, 0.0f, -1.0f};
        
        // UV coordinates
        Vector2 uvs[4] = {
            {0.0f, 0.0f}, // bottom-left
            {1.0f, 0.0f}, // bottom-right
            {1.0f, 1.0f}, // top-right
            {0.0f, 1.0f}  // top-left
        };
        
        // Default color (white)
        Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f};
        
        // Create vertices
        for (int i = 0; i < 4; ++i) {
            Vertex vertex;
            vertex.position = positions[i];
            vertex.normal = normal;
            vertex.color = color;
            vertex.uv = uvs[i];
            mesh.vertices.push_back(vertex);
        }
        
        // Create indices for two triangles (CCW winding)
        mesh.indices = {
            0, 1, 2,
            0, 2, 3
        };
    }
    
    void GeoMeshFactory::createBox(GeoMesh& mesh, float width, float height, float depth)
    {
        // Clear any existing data
        mesh.vertices.clear();
        mesh.indices.clear();
        
        // Half dimensions for easier calculations
        float hw = width * 0.5f;
        float hh = height * 0.5f;
        float hd = depth * 0.5f;
        
        // Define the 8 vertices of the box
        Vector3 positions[8] = {{-hw, -hh, -hd},
                                {hw,  -hh, -hd},
                                {hw,  hh,  -hd},
                                {-hw, hh,  -hd},
                                
                                {-hw, -hh, hd},
                                {hw,  -hh, hd},
                                {hw,  hh,  hd},
                                {-hw, hh,  hd}};
        
        // Define normals for each face
        Vector3 normals[6] = {{0,  0,  -1},  // back
                              {0,  0,  1},   // front
                              {0,  1,  0},   // top
                              {0,  -1, 0},  // bottom
                              {-1, 0,  0},  // left
                              {1,  0,  0}    // right
        };
        
        // UV coordinates (same for all faces)
        Vector2 uvs[4] = {{0, 0},
                          {1, 0},
                          {1, 1},
                          {0, 1}};
        
        // Color (white by default)
        Vector4 color = {1, 1, 1, 1};
        
        // For each face (6 faces total)
        for (int face = 0; face < 6; face++)
        {
            // Determine which vertices belong to this face
            int v0, v1, v2, v3;
            Vector3 normal = normals[face];
            
            switch (face)
            {
                case 0: // back face
                    v0 = 0; v1 = 1; v2 = 2; v3 = 3;
                    break;
                case 1: // front face
                    v0 = 5; v1 = 4; v2 = 7; v3 = 6;
                    break;
                case 2: // top face
                    v0 = 3; v1 = 2; v2 = 6; v3 = 7;
                    break;
                case 3: // bottom face
                    v0 = 4; v1 = 5; v2 = 1; v3 = 0;
                    break;
                case 4: // left face
                    v0 = 0; v1 = 3; v2 = 7; v3 = 4;
                    break;
                case 5: // right face
                    v0 = 1; v1 = 5; v2 = 6; v3 = 2;
                    break;
            }
            
            // Create vertices for this face (4 vertices per face)
            uint32_t baseIndex = mesh.vertices.size();
            mesh.vertices.push_back({positions[v0], normal, color, uvs[0]});
            mesh.vertices.push_back({positions[v1], normal, color, uvs[1]});
            mesh.vertices.push_back({positions[v2], normal, color, uvs[2]});
            mesh.vertices.push_back({positions[v3], normal, color, uvs[3]});
            
            // Add indices for two triangles (CCW winding)
            mesh.indices.push_back(baseIndex + 0);
            mesh.indices.push_back(baseIndex + 1);
            mesh.indices.push_back(baseIndex + 2);
            
            mesh.indices.push_back(baseIndex + 0);
            mesh.indices.push_back(baseIndex + 2);
            mesh.indices.push_back(baseIndex + 3);
        }
    }
    
    void GeoMeshFactory::createSphere(GeoMesh& mesh, float radius, int longitudeSegments, int latitudeSegments)
    {
        mesh.vertices.clear();
        mesh.indices.clear();
        
        // Ensure reasonable parameters
        longitudeSegments = max(3, longitudeSegments);
        latitudeSegments = max(2, latitudeSegments);
        
        // Generate vertices
        for (int lat = 0; lat <= latitudeSegments; ++lat)
        {
            float theta = float(lat) * (M_PI / float(latitudeSegments));
            float sinTheta = sinf(theta);
            float cosTheta = cosf(theta);
            
            for (int lon = 0; lon <= longitudeSegments; ++lon)
            {
                float phi = float(lon) * (2 * M_PI / float(longitudeSegments));
                float sinPhi = sinf(phi);
                float cosPhi = cosf(phi);
                
                Vertex vertex;
                
                // Position (normalized then scaled by radius)
                vertex.position.x = radius * cosPhi * sinTheta;
                vertex.position.y = radius * cosTheta;
                vertex.position.z = radius * sinPhi * sinTheta;
                
                // Normal (same as position but normalized)
                float revRadius = 1.0f / radius;
                vertex.normal.x = vertex.position.x * revRadius;
                vertex.normal.y = vertex.position.y * revRadius;
                vertex.normal.z = vertex.position.z * revRadius;
                
                // Color (white by default)
                vertex.color.x = 1.0f;
                vertex.color.y = 1.0f;
                vertex.color.z = 1.0f;
                vertex.color.w = 1.0f;
                
                // UV coordinates
                vertex.uv.x = float(lon) / float(longitudeSegments);
                vertex.uv.y = float(lat) / float(latitudeSegments);
                
                mesh.vertices.push_back(vertex);
            }
        }
        
        // Generate indices (clockwise winding)
        for (int lat = 0; lat < latitudeSegments; ++lat)
        {
            for (int lon = 0; lon < longitudeSegments; ++lon)
            {
                int row0 = (longitudeSegments + 1) * lat + lon;
                int row1 = (longitudeSegments + 1) * (lat + 1) + lon;
                
                if (lat > 0)
                {
                    mesh.indices.push_back(row0);
                    mesh.indices.push_back(row1);
                    mesh.indices.push_back(row0 + 1);
                }
                
                if (lat < latitudeSegments - 1)
                {
                    mesh.indices.push_back(row0 + 1);
                    mesh.indices.push_back(row1);
                    mesh.indices.push_back(row1 + 1);
                }
            }
        }
    }
    
    void GeoMeshFactory::createCylinder(GeoMesh& mesh, float radius, float height, int longitudeSegments, int latitudeSegments)
    {
        mesh.vertices.clear();
        mesh.indices.clear();
        
        // Ensure reasonable parameters
        longitudeSegments = max(3, longitudeSegments);
        latitudeSegments = max(1, latitudeSegments);
        
        float halfHeight = height * 0.5f;
        
        // Generate side vertices
        for (int lat = 0; lat <= latitudeSegments; ++lat)
        {
            float y = halfHeight - (height * lat) / latitudeSegments;
            
            for (int lon = 0; lon <= longitudeSegments; ++lon)
            {
                float phi = float(lon) * (2 * M_PI / float(longitudeSegments));
                float sinPhi = sinf(phi);
                float cosPhi = cosf(phi);
                
                Vertex vertex;
                
                // Position
                vertex.position.x = radius * cosPhi;
                vertex.position.y = y;
                vertex.position.z = radius * sinPhi;
                
                // Normal (side faces point outward)
                vertex.normal = {cosPhi, 0.0f, sinPhi};
                
                // Color (white by default)
                vertex.color = {1.0f, 1.0f, 1.0f, 1.0f};
                
                // UV coordinates
                vertex.uv.x = float(lon) / float(longitudeSegments);
                vertex.uv.y = float(lat) / float(latitudeSegments);
                
                mesh.vertices.push_back(vertex);
            }
        }
        
        // Generate indices for side faces
        for (int lat = 0; lat < latitudeSegments; ++lat)
        {
            for (int lon = 0; lon < longitudeSegments; ++lon)
            {
                int current = lat * (longitudeSegments + 1) + lon;
                int next = current + longitudeSegments + 1;
                
                mesh.indices.push_back(current);
                mesh.indices.push_back(next + 1);
                mesh.indices.push_back(current + 1);
                
                mesh.indices.push_back(current);
                mesh.indices.push_back(next);
                mesh.indices.push_back(next + 1);
            }
        }
        
        // Generate top and bottom caps
        int baseIndex = mesh.vertices.size();
        
        // Bottom cap center
        mesh.vertices.push_back({
            {0.0f, -halfHeight, 0.0f},  // position
            {0.0f, -1.0f, 0.0f},       // normal
            {1.0f, 1.0f, 1.0f, 1.0f},  // color
            {0.5f, 0.5f}               // uv
        });
        
        // Top cap center
        mesh.vertices.push_back({
            {0.0f, halfHeight, 0.0f},  // position
            {0.0f, 1.0f, 0.0f},        // normal
            {1.0f, 1.0f, 1.0f, 1.0f},  // color
            {0.5f, 0.5f}               // uv
        });
        
        // Generate cap vertices
        for (int lon = 0; lon <= longitudeSegments; ++lon)
        {
            float phi = float(lon) * (2 * M_PI / float(longitudeSegments));
            float sinPhi = sinf(phi);
            float cosPhi = cosf(phi);
            
            // Bottom cap vertex
            mesh.vertices.push_back({
                {radius * cosPhi, -halfHeight, radius * sinPhi}, // position
                {0.0f, -1.0f, 0.0f},                            // normal
                {1.0f, 1.0f, 1.0f, 1.0f},                       // color
                {0.5f + 0.5f * cosPhi, 0.5f + 0.5f * sinPhi}    // uv
            });
            
            // Top cap vertex
            mesh.vertices.push_back({
                {radius * cosPhi, halfHeight, radius * sinPhi},  // position
                {0.0f, 1.0f, 0.0f},                             // normal
                {1.0f, 1.0f, 1.0f, 1.0f},                       // color
                {0.5f + 0.5f * cosPhi, 0.5f + 0.5f * sinPhi}    // uv
            });
        }
        
        // Generate cap indices
        for (int lon = 0; lon < longitudeSegments; ++lon)
        {
            // Bottom cap
            mesh.indices.push_back(baseIndex); // center
            mesh.indices.push_back(baseIndex + 2 + lon * 2 + 2);
            mesh.indices.push_back(baseIndex + 2 + lon * 2);
            
            // Top cap
            mesh.indices.push_back(baseIndex + 1); // center
            mesh.indices.push_back(baseIndex + 2 + lon * 2 + 1);
            mesh.indices.push_back(baseIndex + 2 + lon * 2 + 3);
        }
    }

}
