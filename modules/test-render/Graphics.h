#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <cmath>

#include "render/main.h"

namespace eokas
{
    class Graphics
    {
        Renderer mRenderer;
        Space mSpace;
        StaticMeshPrimitive::Ref mBoxLeft;
        StaticMeshPrimitive::Ref mBoxRight;
        float mAngleY = 0.0f;

    public:
        void init(HWND windowHandle, int32_t windowWidth, int32_t windowHeight)
        {
            mRenderer.create(windowHandle, windowWidth, windowHeight);

            auto camera = std::make_shared<Camera>();
            camera->transform.position = Vector3(0.0f, 0.6f, -2.2f);
            camera->transform.rotation = Quaternion::rotateAxisAngle(
                Vector3(1, 0, 0), atan2f(0.6f, 2.2f));
            camera->aspect = (float)windowWidth / (float)windowHeight;
            camera->fovY = Math::PI / 4.0f;
            camera->viewport.left = 0;
            camera->viewport.top = 0;
            camera->viewport.right = (float)windowWidth;
            camera->viewport.bottom = (float)windowHeight;
            camera->viewport.front = 0.0f;
            camera->viewport.back = 1.0f;
            mSpace.add(camera);
            mSpace.activeCamera = camera;

            auto light = std::make_shared<Light>();
            light->type = LightType::Directional;
            light->intensity = 1.0f;
            light->color = Color(1, 1, 1, 1);
            mSpace.add(light);

            auto matDielectric = std::make_shared<Material>();
            matDielectric->setShaderPath("../shaders/BPR.hlsl");
            matDielectric->setParameter("albedo", Color(0.82f, 0.12f, 0.10f, 1));
            matDielectric->setParameter("metallic", 0.0f);
            matDielectric->setParameter("roughness", 0.75f);

            auto matMetal = std::make_shared<Material>();
            matMetal->setShaderPath("../shaders/BPR.hlsl");
            matMetal->setParameter("albedo", Color(1.0f, 0.78f, 0.34f, 1));
            matMetal->setParameter("metallic", 1.0f);
            matMetal->setParameter("roughness", 0.18f);

            mBoxLeft = std::make_shared<StaticMeshPrimitive>();
            GeoMeshFactory::createBox(mBoxLeft->mesh, 1.0f, 1.0f, 1.0f);
            mBoxLeft->material = matDielectric;
            mBoxLeft->transform.position = Vector3(-0.8f, 0, 0);

            mBoxRight = std::make_shared<StaticMeshPrimitive>();
            GeoMeshFactory::createBox(mBoxRight->mesh, 1.0f, 1.0f, 1.0f);
            mBoxRight->material = matMetal;
            mBoxRight->transform.position = Vector3(0.8f, 0, 0);

            mSpace.add(mBoxLeft);
            mSpace.add(mBoxRight);
        }

        void quit()
        {
        }

        void tick(float delta)
        {
            float dt = delta > 0.0f ? delta : (1.0f / 60.0f);
            if (dt > 0.1f) dt = 0.1f;
            mAngleY += Math::PI * 2.0f * dt;

            Quaternion rot = Quaternion::rotateAxisAngle(Vector3(0, 1, 0), mAngleY);
            if (mBoxLeft)
                mBoxLeft->transform.rotation = rot;
            if (mBoxRight)
                mBoxRight->transform.rotation = rot;

            mRenderer.render(mSpace);
        }
    };
}
