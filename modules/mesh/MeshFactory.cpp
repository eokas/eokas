#include "./MeshFactory.h"

namespace eokas
{
    static const float M_PI = 3.141592653f;
    
    void MeshFactory::createQuad(RawMesh& mesh, float width, float height)
    {
        // Clear any existing data
        mesh.clear();
        
        // Half dimensions for centered quad
        float halfW = width * 0.5f;
        float halfH = height * 0.5f;
        
        // Define vertex positions (facing -Z)
        Vector3 positions[4] = {{-halfW, halfH,  0.0f},
                                {halfW,  halfH,  0.0f},
                                {halfW,  -halfH, 0.0f},
                                {-halfW, -halfH, 0.0f}};
        
        // Normal facing -Z (towards screen)
        Vector3 normal = {0.0f, 0.0f, -1.0f};
        
        // UV coordinates
        Vector2 uvs[4] = {{0.0f, 0.0f}, // bottom-left
                          {1.0f, 0.0f}, // bottom-right
                          {1.0f, 1.0f}, // top-right
                          {0.0f, 1.0f}  // top-left
        };
        
        // Default color (white)
        Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f};
        
        // Add attributes for normals, UVs, and colors
        AttributeID normalAttrId = mesh.addAttribute<Vector3>(RawMesh::AttributeUsage::Normal);
        AttributeID uvAttrId = mesh.addAttribute<Vector2>(RawMesh::AttributeUsage::UV);
        AttributeID colorAttrId = mesh.addAttribute<Vector4>(RawMesh::AttributeUsage::Color);
        
        // Get references to the attributes
        auto& normalAttr = mesh.getAttribute<Vector3>(normalAttrId);
        auto& uvAttr = mesh.getAttribute<Vector2>(uvAttrId);
        auto& colorAttr = mesh.getAttribute<Vector4>(colorAttrId);
        
        // Create a default section for the quad
        SectionID sectionId = mesh.addSection();
        
        // Add vertices to the mesh
        std::vector<VertexID> vertexIds;
        for (const auto& pos: positions)
        {
            vertexIds.push_back(mesh.addVertex({pos}));
        }
        
        // Add corners (one per vertex)
        std::vector<CornerID> cornerIds = mesh.addCorners(vertexIds);
        
        // Set attributes for each corner
        for (size_t i = 0; i < cornerIds.size(); i++)
        {
            normalAttr.set(cornerIds[i], normal);
            uvAttr.set(cornerIds[i], uvs[i]);
            colorAttr.set(cornerIds[i], color);
        }
        
        // Create two triangles for the quad (CCW winding)
        RawMesh::Triangle triangle1 = {cornerIds[0], cornerIds[1], cornerIds[2]};
        RawMesh::Triangle triangle2 = {cornerIds[0], cornerIds[2], cornerIds[3]};
        
        // Add triangles to the mesh and section
        TriangleID triId1 = mesh.addTriangle(triangle1);
        TriangleID triId2 = mesh.addTriangle(triangle2);
        
        mesh.addSectionTriangle(sectionId, triId1);
        mesh.addSectionTriangle(sectionId, triId2);
    }
    
    void MeshFactory::createBox(RawMesh& mesh, float width, float height, float depth)
    {
        // Clear any existing data
        mesh.clear();
        
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
                              {0,  -1, 0},   // bottom
                              {-1, 0,  0},   // left
                              {1,  0,  0}    // right
        };
        
        // UV coordinates (same for all faces)
        Vector2 uvs[4] = {{0, 0},
                          {1, 0},
                          {1, 1},
                          {0, 1}};
        
        // Color (white by default)
        Vector4 color = {1, 1, 1, 1};
        
        // Add attributes for normals, UVs, and colors
        AttributeID normalAttrId = mesh.addAttribute<Vector3>(RawMesh::AttributeUsage::Normal);
        AttributeID uvAttrId = mesh.addAttribute<Vector2>(RawMesh::AttributeUsage::UV);
        AttributeID colorAttrId = mesh.addAttribute<Vector4>(RawMesh::AttributeUsage::Color);
        
        // Get references to the attributes
        auto& normalAttr = mesh.getAttribute<Vector3>(normalAttrId);
        auto& uvAttr = mesh.getAttribute<Vector2>(uvAttrId);
        auto& colorAttr = mesh.getAttribute<Vector4>(colorAttrId);
        
        // Create a default section for the box
        SectionID sectionId = mesh.addSection();
        
        // For each face (6 faces total)
        for (int face = 0; face < 6; face++)
        {
            // Determine which vertices belong to this face
            int v0, v1, v2, v3;
            Vector3 normal = normals[face];
            
            switch (face)
            {
                case 0: // back face
                    v0 = 0;
                    v1 = 1;
                    v2 = 2;
                    v3 = 3;
                    break;
                case 1: // front face
                    v0 = 5;
                    v1 = 4;
                    v2 = 7;
                    v3 = 6;
                    break;
                case 2: // top face
                    v0 = 3;
                    v1 = 2;
                    v2 = 6;
                    v3 = 7;
                    break;
                case 3: // bottom face
                    v0 = 4;
                    v1 = 5;
                    v2 = 1;
                    v3 = 0;
                    break;
                case 4: // left face
                    v0 = 0;
                    v1 = 3;
                    v2 = 7;
                    v3 = 4;
                    break;
                case 5: // right face
                    v0 = 1;
                    v1 = 5;
                    v2 = 6;
                    v3 = 2;
                    break;
            }
            
            // Add vertices to the mesh (if not already added)
            RawMesh::Vertex vertices[4] = {{positions[v0]},
                                           {positions[v1]},
                                           {positions[v2]},
                                           {positions[v3]}};
            
            std::vector<VertexID> vertexIds;
            for (const auto& vertex: vertices)
            {
                vertexIds.push_back(mesh.addVertex(vertex));
            }
            
            // Add corners (one per vertex per face)
            std::vector<CornerID> cornerIds = mesh.addCorners(vertexIds);
            
            // Set attributes for each corner
            for (size_t i = 0; i < cornerIds.size(); i++)
            {
                normalAttr.set(cornerIds[i], normal);
                uvAttr.set(cornerIds[i], uvs[i]);
                colorAttr.set(cornerIds[i], color);
            }
            
            // Create two triangles for the face
            RawMesh::Triangle triangle1 = {cornerIds[0], cornerIds[1], cornerIds[2]};
            RawMesh::Triangle triangle2 = {cornerIds[0], cornerIds[2], cornerIds[3]};
            
            // Add triangles to the mesh and section
            TriangleID triId1 = mesh.addTriangle(triangle1);
            TriangleID triId2 = mesh.addTriangle(triangle2);
            
            mesh.addSectionTriangle(sectionId, triId1);
            mesh.addSectionTriangle(sectionId, triId2);
        }
    }
    
    void MeshFactory::createSphere(RawMesh& mesh, float radius, int longitudeSegments, int latitudeSegments)
    {
        // Clear any existing data
        mesh.clear();
        
        // Ensure reasonable parameters
        longitudeSegments = max(3, longitudeSegments);
        latitudeSegments = max(2, latitudeSegments);
        
        // Add attributes for normals, UVs, and colors
        AttributeID normalAttrId = mesh.addAttribute<Vector3>(RawMesh::AttributeUsage::Normal);
        AttributeID uvAttrId = mesh.addAttribute<Vector2>(RawMesh::AttributeUsage::UV);
        AttributeID colorAttrId = mesh.addAttribute<Vector4>(RawMesh::AttributeUsage::Color);
        
        // Get references to the attributes
        auto& normalAttr = mesh.getAttribute<Vector3>(normalAttrId);
        auto& uvAttr = mesh.getAttribute<Vector2>(uvAttrId);
        auto& colorAttr = mesh.getAttribute<Vector4>(colorAttrId);
        
        // Create a default section for the sphere
        SectionID sectionId = mesh.addSection();
        
        // Generate vertices and corners
        std::vector<VertexID> vertexIds;
        std::vector<CornerID> cornerIds;
        
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
                
                // Create vertex position
                Vector3 position;
                position.x = radius * cosPhi * sinTheta;
                position.y = radius * cosTheta;
                position.z = radius * sinPhi * sinTheta;
                
                // Add vertex
                RawMesh::Vertex vertex;
                vertex.pos = position;
                VertexID vertexId = mesh.addVertex(vertex);
                vertexIds.push_back(vertexId);
                
                // Add corner
                CornerID cornerId = mesh.addCorner(vertexId);
                cornerIds.push_back(cornerId);
                
                // Set attributes
                Vector3 normal = position;
                float revRadius = 1.0f / radius;
                normal.x *= revRadius;
                normal.y *= revRadius;
                normal.z *= revRadius;
                
                Vector2 uv;
                uv.x = float(lon) / float(longitudeSegments);
                uv.y = float(lat) / float(latitudeSegments);
                
                Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f};
                
                normalAttr.set(cornerId, normal);
                uvAttr.set(cornerId, uv);
                colorAttr.set(cornerId, color);
            }
        }
        
        // Generate triangles (clockwise winding)
        for (int lat = 0; lat < latitudeSegments; ++lat)
        {
            for (int lon = 0; lon < longitudeSegments; ++lon)
            {
                int row0 = (longitudeSegments + 1) * lat + lon;
                int row1 = (longitudeSegments + 1) * (lat + 1) + lon;
                
                if (lat > 0)
                {
                    RawMesh::Triangle triangle1 = {cornerIds[row0], cornerIds[row1], cornerIds[row0 + 1]};
                    TriangleID triId1 = mesh.addTriangle(triangle1);
                    mesh.addSectionTriangle(sectionId, triId1);
                }
                
                if (lat < latitudeSegments - 1)
                {
                    RawMesh::Triangle triangle2 = {cornerIds[row0 + 1], cornerIds[row1], cornerIds[row1 + 1]};
                    TriangleID triId2 = mesh.addTriangle(triangle2);
                    mesh.addSectionTriangle(sectionId, triId2);
                }
            }
        }
    }
    
    void MeshFactory::createCylinder(RawMesh& mesh, float radius, float height, int longitudeSegments, int latitudeSegments)
    {
        // Clear any existing data
        mesh.clear();
        
        // Ensure reasonable parameters
        longitudeSegments = max(3, longitudeSegments);
        latitudeSegments = max(1, latitudeSegments);
        
        float halfHeight = height * 0.5f;
        
        // Add attributes for normals, UVs, and colors
        AttributeID normalAttrId = mesh.addAttribute<Vector3>(RawMesh::AttributeUsage::Normal);
        AttributeID uvAttrId = mesh.addAttribute<Vector2>(RawMesh::AttributeUsage::UV);
        AttributeID colorAttrId = mesh.addAttribute<Vector4>(RawMesh::AttributeUsage::Color);
        
        // Get references to the attributes
        auto& normalAttr = mesh.getAttribute<Vector3>(normalAttrId);
        auto& uvAttr = mesh.getAttribute<Vector2>(uvAttrId);
        auto& colorAttr = mesh.getAttribute<Vector4>(colorAttrId);
        
        // Create a default section for the cylinder
        SectionID sectionId = mesh.addSection();
        
        // Generate side vertices and corners
        std::vector<VertexID> vertexIds;
        std::vector<CornerID> cornerIds;
        
        for (int lat = 0; lat <= latitudeSegments; ++lat)
        {
            float y = halfHeight - (height * lat) / latitudeSegments;
            
            for (int lon = 0; lon <= longitudeSegments; ++lon)
            {
                float phi = float(lon) * (2 * M_PI / float(longitudeSegments));
                float sinPhi = sinf(phi);
                float cosPhi = cosf(phi);
                
                // Create vertex position
                Vector3 position;
                position.x = radius * cosPhi;
                position.y = y;
                position.z = radius * sinPhi;
                
                // Add vertex
                RawMesh::Vertex vertex;
                vertex.pos = position;
                VertexID vertexId = mesh.addVertex(vertex);
                vertexIds.push_back(vertexId);
                
                // Add corner
                CornerID cornerId = mesh.addCorner(vertexId);
                cornerIds.push_back(cornerId);
                
                // Set attributes
                Vector3 normal = {cosPhi, 0.0f, sinPhi};
                Vector2 uv = {float(lon) / float(longitudeSegments), float(lat) / float(latitudeSegments)};
                Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f};
                
                normalAttr.set(cornerId, normal);
                uvAttr.set(cornerId, uv);
                colorAttr.set(cornerId, color);
            }
        }
        
        // Generate triangles for side faces
        for (int lat = 0; lat < latitudeSegments; ++lat)
        {
            for (int lon = 0; lon < longitudeSegments; ++lon)
            {
                int current = lat * (longitudeSegments + 1) + lon;
                int next = current + longitudeSegments + 1;
                
                // First triangle
                RawMesh::Triangle triangle1 = {cornerIds[current], cornerIds[next + 1], cornerIds[current + 1]};
                TriangleID triId1 = mesh.addTriangle(triangle1);
                mesh.addSectionTriangle(sectionId, triId1);
                
                // Second triangle
                RawMesh::Triangle triangle2 = {cornerIds[current], cornerIds[next], cornerIds[next + 1]};
                TriangleID triId2 = mesh.addTriangle(triangle2);
                mesh.addSectionTriangle(sectionId, triId2);
            }
        }
        
        // Generate top and bottom caps
        // Bottom cap center
        RawMesh::Vertex bottomCenterVertex;
        bottomCenterVertex.pos = {0.0f, -halfHeight, 0.0f};
        VertexID bottomCenterId = mesh.addVertex(bottomCenterVertex);
        CornerID bottomCenterCornerId = mesh.addCorner(bottomCenterId);
        
        // Top cap center
        RawMesh::Vertex topCenterVertex;
        topCenterVertex.pos = {0.0f, halfHeight, 0.0f};
        VertexID topCenterId = mesh.addVertex(topCenterVertex);
        CornerID topCenterCornerId = mesh.addCorner(topCenterId);
        
        // Set attributes for center points
        Vector3 bottomNormal = {0.0f, -1.0f, 0.0f};
        Vector3 topNormal = {0.0f, 1.0f, 0.0f};
        Vector2 centerUV = {0.5f, 0.5f};
        Vector4 whiteColor = {1.0f, 1.0f, 1.0f, 1.0f};
        
        normalAttr.set(bottomCenterCornerId, bottomNormal);
        uvAttr.set(bottomCenterCornerId, centerUV);
        colorAttr.set(bottomCenterCornerId, whiteColor);
        
        normalAttr.set(topCenterCornerId, topNormal);
        uvAttr.set(topCenterCornerId, centerUV);
        colorAttr.set(topCenterCornerId, whiteColor);
        
        // Generate cap vertices and corners
        std::vector<CornerID> bottomCapCornerIds;
        std::vector<CornerID> topCapCornerIds;
        
        for (int lon = 0; lon <= longitudeSegments; ++lon)
        {
            float phi = float(lon) * (2 * M_PI / float(longitudeSegments));
            float sinPhi = sinf(phi);
            float cosPhi = cosf(phi);
            
            // Bottom cap vertex
            RawMesh::Vertex bottomVertex;
            bottomVertex.pos = {radius * cosPhi, -halfHeight, radius * sinPhi};
            VertexID bottomVertexId = mesh.addVertex(bottomVertex);
            CornerID bottomCornerId = mesh.addCorner(bottomVertexId);
            bottomCapCornerIds.push_back(bottomCornerId);
            
            // Set attributes for bottom cap vertex
            Vector2 bottomUV = {0.5f + 0.5f * cosPhi, 0.5f + 0.5f * sinPhi};
            
            normalAttr.set(bottomCornerId, bottomNormal);
            uvAttr.set(bottomCornerId, bottomUV);
            colorAttr.set(bottomCornerId, whiteColor);
            
            // Top cap vertex
            RawMesh::Vertex topVertex;
            topVertex.pos = {radius * cosPhi, halfHeight, radius * sinPhi};
            VertexID topVertexId = mesh.addVertex(topVertex);
            CornerID topCornerId = mesh.addCorner(topVertexId);
            topCapCornerIds.push_back(topCornerId);
            
            // Set attributes for top cap vertex
            Vector2 topUV = {0.5f + 0.5f * cosPhi, 0.5f + 0.5f * sinPhi};
            
            normalAttr.set(topCornerId, topNormal);
            uvAttr.set(topCornerId, topUV);
            colorAttr.set(topCornerId, whiteColor);
        }
        
        // Generate cap triangles
        for (int lon = 0; lon < longitudeSegments; ++lon)
        {
            // Bottom cap triangle
            RawMesh::Triangle bottomTriangle = {bottomCenterCornerId, bottomCapCornerIds[lon + 1], bottomCapCornerIds[lon]};
            TriangleID bottomTriId = mesh.addTriangle(bottomTriangle);
            mesh.addSectionTriangle(sectionId, bottomTriId);
            
            // Top cap triangle
            RawMesh::Triangle topTriangle = {topCenterCornerId, topCapCornerIds[lon], topCapCornerIds[lon + 1]};
            TriangleID topTriId = mesh.addTriangle(topTriangle);
            mesh.addSectionTriangle(sectionId, topTriId);
        }
    }
}