#include "Unit.h"
#include "render/main.h"

using namespace eokas;

EOKAS_TEST_CASE(render) {
    Transform transform;
    transform.position = Vector3(1, 2, 3);
    EOKAS_EXPECT(transform.forward() == Vector3::FORWARD);
    EOKAS_EXPECT(transform.up() == Vector3::UP);
    EOKAS_EXPECT(transform.right() == Vector3::RIGHT);
    Matrix4 world = transform.toMatrix();
    EOKAS_EXPECT(!_FloatEqual(world.value[3][0] + world.value[3][1] + world.value[3][2], 0.0f) || world != Matrix4::ZERO);

    Camera camera;
    camera.transform.position = Vector3(0, 0, -5);
    Matrix4 view = camera.viewMatrix();
    Matrix4 projection = camera.projectionMatrix();
    EOKAS_EXPECT(view != Matrix4::ZERO);
    EOKAS_EXPECT(projection != Matrix4::ZERO);
    EOKAS_EXPECT(camera.viewProjectionMatrix() != projection);
    camera.perspective = false;
    EOKAS_EXPECT(camera.projectionMatrix() != projection);

    Light light;
    light.type = LightType::Point;
    light.intensity = 2.0f;
    light.color = Color(0.2f, 0.4f, 0.6f, 1.0f);
    light.transform.position = Vector3(1, 2, 3);
    EOKAS_EXPECT(light.direction() == Vector3::FORWARD);
    LightUniforms gpu{};
    light.fillGpu(gpu);
    EOKAS_EXPECT(_FloatEqual(gpu.colorIntensity[3], 2.0f));
    EOKAS_EXPECT(_FloatEqual(gpu.posRange[0], 1.0f));
    EOKAS_EXPECT(_FloatEqual(gpu.typeInner[0], 1.0f));

    Space space;
    Camera::Ref added = std::make_shared<Camera>();
    Light::Ref lamp = std::make_shared<Light>();
    space.add(added);
    space.add(lamp);
    space.activeCamera = added;
    EOKAS_EXPECT(space.cameras.size() == 1);
    EOKAS_EXPECT(space.lights.size() == 1);
    space.remove(added);
    EOKAS_EXPECT(space.cameras.empty());
    EOKAS_EXPECT(space.activeCamera == nullptr);

    Material material;
    EOKAS_EXPECT(!material.isReady());
    EOKAS_EXPECT(material.setParameter("metallic", 0.25f));
    float metallic = 0.0f;
    EOKAS_EXPECT(material.getParameter("metallic", metallic));
    EOKAS_EXPECT(_FloatEqual(metallic, 0.25f));
    EOKAS_EXPECT(material.setParameter("albedo", Color(0.1f, 0.2f, 0.3f, 1.0f)));
    Color albedo;
    EOKAS_EXPECT(material.getParameter("albedo", albedo));
    EOKAS_EXPECT(_FloatEqual(albedo.r, 0.1f));
    EOKAS_EXPECT(!material.setParameter("missing", 1.0f));
    return 0;
}
