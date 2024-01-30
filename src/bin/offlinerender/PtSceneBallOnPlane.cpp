#include "PtSceneBallOnPlane.hpp"

#include "SceneUtils.hpp"

PtSceneBallOnPlane::PtSceneBallOnPlane(vengine::Engine &engine)
    : PtScene(engine){};

bool PtSceneBallOnPlane::create()
{
    auto camera = std::make_shared<vengine::PerspectiveCamera>();
    camera->transform().position() = glm::vec3(0, 3, 10);
    camera->transform().setRotation(glm::quat(glm::vec3(glm::radians(-15.F), 0, 0)));
    camera->fov() = 60.0f;
    camera->lensRadius() = 0.03F;
    camera->focalDistance() = 7.0F;
    scene().camera() = camera;

    addModel3D(scene(), nullptr, "plane", "assets/models/plane.obj", vengine::Transform({0, -3, 0}, {10, 10, 10}), "defaultMaterial");
    addModel3D(scene(), nullptr, "sphere", "assets/models/uvsphere.obj", vengine::Transform({0, 0, 0}, {3, 3, 3}), "defaultMaterial");

    auto &instanceLights = vengine::AssetManager::getInstance().lightsMap();
    vengine::SceneObject *lightObject = scene().addSceneObject("light", vengine::Transform({4, 1.5, 0}));
    lightObject->add<vengine::ComponentLight>().light = instanceLights.get("defaultPointLight");

    scene().environmentIntensity() = 1.0F;

    scene().updateSceneGraph();
    return true;
}

bool PtSceneBallOnPlane::render()
{
    /* Performs a render sequence */

    engine().renderer().rendererPathTracing().renderInfo().samples = 64;
    engine().renderer().rendererPathTracing().renderInfo().batchSize = 64;
    engine().renderer().rendererPathTracing().renderInfo().fileType = vengine::FileType::PNG;
    engine().renderer().rendererPathTracing().renderInfo().denoise = true;
    engine().renderer().rendererPathTracing().renderInfo().writeAllFiles = false;

    uint32_t i = 0;
    float height = 1.0F;
    float radius = 10.0F;
    float angleStep = glm::radians(45.0F);
    for (float a = 0; a < 2 * M_PI; a += angleStep) {
        engine().renderer().rendererPathTracing().renderInfo().filename = std::to_string(i++);

        scene().camera()->transform().position() = glm::vec3(radius * sin(a), height, radius * cos(a));
        scene().camera()->transform().setRotationEuler(0, a, 0);

        engine().renderer().rendererPathTracing().render();
    }

    return true;
}