#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <cmath>

#include "Render/Main.h"

namespace eokas
{
    class Graphics
    {
        Renderer mRenderer;
        Surface::Ref mSurface;
        Space mSpace;
        Camera::Ref mCamera;
        StaticMeshPrimitive::Ref mSkybox;
        HWND mWindow = nullptr;
        float mYaw = 0.0f;
        float mPitch = 18.0f;
        bool mLooking = false;
        POINT mLookAnchor{};

        static constexpr float kTerrainSize = 96.0f;
        static constexpr int kTerrainSegments = 80;
        static constexpr float kMoveSpeed = 18.0f;
        static constexpr float kLookSensitivity = 0.15f;

    public:
        void init(HWND windowHandle, int32_t windowWidth, int32_t windowHeight)
        {
            mWindow = windowHandle;
            mRenderer.create();
            mSurface = mRenderer.attach(windowHandle, (uint32_t)windowWidth, (uint32_t)windowHeight);

            mSpace.clearColor = Color(0.45f, 0.70f, 0.95f, 1.0f);
            mSpace.ambient = Color(0.18f, 0.20f, 0.24f, 1.0f);
            mSpace.shadowCenter = Vector3(0.0f, 4.0f, 0.0f);
            mSpace.shadowExtent = 150.0f;

            mCamera = std::make_shared<Camera>();
            mCamera->transform.position = Vector3(0.0f, sampleHeight(0.0f, -28.0f) + 7.0f, -28.0f);
            mCamera->aspect = (float)windowWidth / (float)windowHeight;
            mCamera->fovY = Math::PI / 3.0f;
            mCamera->nearZ = 0.1f;
            mCamera->farZ = 800.0f;
            mCamera->viewport.left = 0;
            mCamera->viewport.top = 0;
            mCamera->viewport.right = (float)windowWidth;
            mCamera->viewport.bottom = (float)windowHeight;
            mCamera->viewport.front = 0.0f;
            mCamera->viewport.back = 1.0f;
            applyCameraRotation();
            mSpace.add(mCamera);
            mSpace.activeCamera = mCamera;

            auto light = std::make_shared<Light>();
            light->type = LightType::Directional;
            light->intensity = 2.2f;
            light->color = Color(1.0f, 0.94f, 0.78f, 1.0f);
            Quaternion sunPitch = Quaternion::rotateAxisAngle(Vector3(1, 0, 0), 36.0f);
            Quaternion sunYaw = Quaternion::rotateAxisAngle(Vector3(0, 1, 0), -55.0f);
            light->transform.rotation = mulQuat(sunYaw, sunPitch);
            mSpace.add(light);

            mSkybox = std::make_shared<StaticMeshPrimitive>();
            GeoMeshFactory::createBox(mSkybox->mesh, 1.0f, 1.0f, 1.0f);
            mSkybox->material = createMaterial(
                Color(0.0f, 0.0f, 0.0f, 1.0f),
                0.0f,
                1.0f,
                Color(0.45f, 0.70f, 0.95f, 1.0f));
            mSkybox->material->setCullMode(CullMode::Back);
            DepthStencilState skyDepth;
            skyDepth.depthTest = true;
            skyDepth.depthWrite = false;
            skyDepth.depthFunc = CompareOp::Always;
            mSkybox->material->setDepthStencilState(skyDepth);
            mSkybox->transform.scale = Vector3(200.0f, 200.0f, 200.0f);
            mSkybox->transform.position = mCamera->transform.position;
            mSpace.add(mSkybox);

            auto terrain = std::make_shared<StaticMeshPrimitive>();
            createHeightTerrain(terrain->mesh);
            terrain->material = createMaterial(Color(0.72f, 0.80f, 0.58f, 1.0f), 0.0f, 0.88f);
            terrain->receiveShadow = true;
            terrain->transform.position = Vector3(0, 0, 0);
            mSpace.add(terrain);

            GeoMesh cylinder;
            GeoMeshFactory::createCylinder(cylinder, 1.0f, 1.0f, 12, 1);
            auto trunkMat = createMaterial(Color(0.42f, 0.26f, 0.12f, 1.0f), 0.0f, 0.86f);
            auto crownMat = createMaterial(Color(0.16f, 0.48f, 0.18f, 1.0f), 0.0f, 0.72f);

            const int treeCount = 22;
            for (int i = 0; i < treeCount; ++i)
            {
                float u = hash01(i * 2 + 1);
                float v = hash01(i * 2 + 7);
                float x = (u - 0.5f) * kTerrainSize * 0.78f;
                float z = (v - 0.5f) * kTerrainSize * 0.78f;
                float ground = sampleHeight(x, z);
                float trunkH = 2.4f + hash01(i * 3 + 3) * 2.8f;
                float trunkR = 0.22f + hash01(i * 5 + 1) * 0.16f;
                float crownH = 1.6f + hash01(i * 4 + 2) * 1.4f;
                float crownR = 0.9f + hash01(i * 6 + 4) * 0.8f;

                auto trunk = std::make_shared<StaticMeshPrimitive>();
                trunk->mesh = cylinder;
                trunk->material = trunkMat;
                trunk->castShadow = true;
                trunk->receiveShadow = true;
                trunk->transform.scale = Vector3(trunkR, trunkH, trunkR);
                trunk->transform.position = Vector3(x, ground + trunkH * 0.5f, z);
                mSpace.add(trunk);

                auto crown = std::make_shared<StaticMeshPrimitive>();
                crown->mesh = cylinder;
                crown->material = crownMat;
                crown->castShadow = true;
                crown->receiveShadow = true;
                crown->transform.scale = Vector3(crownR, crownH, crownR);
                crown->transform.position = Vector3(x, ground + trunkH + crownH * 0.32f, z);
                mSpace.add(crown);
            }

            GeoMesh cloudMesh;
            GeoMeshFactory::createBox(cloudMesh, 1.0f, 1.0f, 1.0f);
            auto cloudMat = createMaterial(Color(0.0f, 0.0f, 0.0f, 1.0f), 0.0f, 1.0f, Color(1.0f, 1.0f, 1.0f, 1.0f));
            const Vector3 clusters[] = {
                Vector3(-22.0f, 18.0f, 6.0f),
                Vector3(8.0f, 22.0f, 18.0f),
                Vector3(26.0f, 16.5f, -8.0f),
                Vector3(-6.0f, 20.0f, 32.0f),
                Vector3(18.0f, 24.0f, 40.0f),
                Vector3(-30.0f, 17.0f, 24.0f),
                Vector3(2.0f, 19.0f, -12.0f),
            };
            int cloudId = 0;
            for (const Vector3& cluster : clusters)
            {
                for (int part = 0; part < 5; ++part)
                {
                    float ox = (hash01(cloudId * 3 + 1) - 0.5f) * 7.0f;
                    float oy = (hash01(cloudId * 3 + 2) - 0.5f) * 1.4f;
                    float oz = (hash01(cloudId * 3 + 3) - 0.5f) * 4.5f;
                    float sx = 2.2f + hash01(cloudId * 5 + 4) * 3.4f;
                    float sy = 0.7f + hash01(cloudId * 5 + 5) * 0.9f;
                    float sz = 1.6f + hash01(cloudId * 5 + 6) * 2.4f;
                    auto puff = std::make_shared<StaticMeshPrimitive>();
                    puff->mesh = cloudMesh;
                    puff->material = cloudMat;
                    puff->transform.position = Vector3(cluster.x + ox, cluster.y + oy, cluster.z + oz);
                    puff->transform.scale = Vector3(sx, sy, sz);
                    mSpace.add(puff);
                    cloudId += 1;
                }
            }
        }

        void quit()
        {
            if (mLooking)
            {
                mLooking = false;
                ShowCursor(TRUE);
            }
            mRenderer.detach(mSurface);
        }

        void tick(float delta)
        {
            float dt = delta > 0.0f ? delta : (1.0f / 60.0f);
            if (dt > 0.1f)
                dt = 0.1f;

            updateCamera(dt);
            if (mSkybox && mCamera)
                mSkybox->transform.position = mCamera->transform.position;

            mRenderer.render(mSurface, mSpace);
        }

    private:
        static Quaternion mulQuat(const Quaternion& a, const Quaternion& b)
        {
            return Quaternion(
                a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
                a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
                a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
                a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z);
        }

        static float hash01(int n)
        {
            float s = sinf((float)n * 127.1f) * 43758.5453f;
            return s - floorf(s);
        }

        static float sampleHeight(float x, float z)
        {
            float h = 0.0f;
            h += 3.4f * sinf(x * 0.11f) * cosf(z * 0.09f);
            h += 1.7f * sinf(x * 0.045f + z * 0.038f);
            h += 0.85f * cosf(x * 0.28f) * sinf(z * 0.24f);
            return h;
        }

        static void createHeightTerrain(GeoMesh& mesh)
        {
            mesh.vertices.clear();
            mesh.indices.clear();

            const int n = kTerrainSegments;
            const float step = kTerrainSize / (float)n;
            const float origin = -kTerrainSize * 0.5f;

            for (int iz = 0; iz <= n; ++iz)
            {
                for (int ix = 0; ix <= n; ++ix)
                {
                    float x = origin + ix * step;
                    float z = origin + iz * step;
                    float y = sampleHeight(x, z);

                    float hL = sampleHeight(x - step, z);
                    float hR = sampleHeight(x + step, z);
                    float hD = sampleHeight(x, z - step);
                    float hU = sampleHeight(x, z + step);
                    Vector3 normal(hL - hR, 2.0f * step, hD - hU);
                    normal.normalize();

                    float t = Math::clamp((y + 3.0f) / 8.0f, 0.0f, 1.0f);
                    Vertex vertex;
                    vertex.position = Vector3(x, y, z);
                    vertex.normal = normal;
                    vertex.color = Vector4(
                        Math::lerp(0.55f, 0.95f, t),
                        Math::lerp(0.62f, 1.0f, t),
                        Math::lerp(0.32f, 0.62f, t),
                        1.0f);
                    vertex.uv = Vector2((float)ix / (float)n, (float)iz / (float)n);
                    mesh.vertices.push_back(vertex);
                }
            }

            for (int iz = 0; iz < n; ++iz)
            {
                for (int ix = 0; ix < n; ++ix)
                {
                    uint32_t i00 = (uint32_t)(iz * (n + 1) + ix);
                    uint32_t i10 = i00 + 1;
                    uint32_t i01 = i00 + (uint32_t)(n + 1);
                    uint32_t i11 = i01 + 1;
                    mesh.indices.push_back(i00);
                    mesh.indices.push_back(i10);
                    mesh.indices.push_back(i11);
                    mesh.indices.push_back(i00);
                    mesh.indices.push_back(i11);
                    mesh.indices.push_back(i01);
                }
            }
        }

        static Material::Ref createMaterial(const Color& albedo, float metallic, float roughness, const Color& emissive = Color(0, 0, 0, 1))
        {
            auto material = std::make_shared<Material>();
            material->setShaderPath("../Content/Shaders/PBR.hlsl");
            material->setParameter("albedo", albedo);
            material->setParameter("metallic", metallic);
            material->setParameter("roughness", roughness);
            material->setParameter("emissive", emissive);
            return material;
        }

        void applyCameraRotation()
        {
            Quaternion pitch = Quaternion::rotateAxisAngle(Vector3(1, 0, 0), mPitch);
            Quaternion yaw = Quaternion::rotateAxisAngle(Vector3(0, 1, 0), mYaw);
            mCamera->transform.rotation = mulQuat(yaw, pitch);
        }

        void updateCamera(float dt)
        {
            if (!mCamera)
                return;

            bool focused = mWindow && GetForegroundWindow() == mWindow;
            bool rmb = focused && (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
            if (rmb)
            {
                if (!mLooking)
                {
                    mLooking = true;
                    GetCursorPos(&mLookAnchor);
                    ShowCursor(FALSE);
                }
                else
                {
                    POINT now{};
                    GetCursorPos(&now);
                    float dx = (float)(now.x - mLookAnchor.x);
                    float dy = (float)(now.y - mLookAnchor.y);
                    if (dx != 0.0f || dy != 0.0f)
                    {
                        mYaw += dx * kLookSensitivity;
                        mPitch = Math::clamp(mPitch + dy * kLookSensitivity, -89.0f, 89.0f);
                        SetCursorPos(mLookAnchor.x, mLookAnchor.y);
                    }
                }
            }
            else if (mLooking)
            {
                mLooking = false;
                ShowCursor(TRUE);
            }

            applyCameraRotation();

            if (!focused)
                return;

            Vector3 move = Vector3(0, 0, 0);
            Vector3 forward = mCamera->transform.forward();
            Vector3 right = mCamera->transform.right();
            if (GetAsyncKeyState('W') & 0x8000)
                move += forward;
            if (GetAsyncKeyState('S') & 0x8000)
                move -= forward;
            if (GetAsyncKeyState('D') & 0x8000)
                move += right;
            if (GetAsyncKeyState('A') & 0x8000)
                move -= right;
            if (move.sqrmagnitude() > 1.0e-6f)
                mCamera->transform.position += move.normalized() * (kMoveSpeed * dt);
        }
    };
}
