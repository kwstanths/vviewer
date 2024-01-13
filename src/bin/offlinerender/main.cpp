#include <iostream>

#include "vengine/vulkan/VulkanEngine.hpp"

using namespace vengine;

void createSceneBallOnPlane(Scene &scene)
{
    auto camera = std::make_shared<PerspectiveCamera>();
    camera->transform().position() = glm::vec3(0, 3, 10);
    camera->transform().setRotation(glm::quat(glm::vec3(glm::radians(-15.F), 0, 0)));
    camera->fov() = 60.0f;
    camera->lensRadius() = 0.03F;
    camera->focalDistance() = 7.0F;
    scene.camera() = camera;

    auto &instanceModels = AssetManager::getInstance().modelsMap();
    auto &instanceMaterials = AssetManager::getInstance().materialsMap();
    auto &instanceLights = AssetManager::getInstance().lightsMap();

    auto matDef = instanceMaterials.get("defaultMaterial");
    auto plane = instanceModels.get("assets/models/plane.obj");
    auto sphere = instanceModels.get("assets/models/uvsphere.obj");

    SceneObject *planeObject = scene.addSceneObject("plane", Transform({0, -3, 0}, {10, 10, 10}));
    planeObject->add<ComponentMesh>().mesh = plane->mesh("Plane");
    planeObject->add<ComponentMaterial>().material = matDef;

    SceneObject *sphereObject = scene.addSceneObject("sphere", Transform({0, 0, 0}, {3, 3, 3}));
    sphereObject->add<ComponentMesh>().mesh = sphere->mesh("defaultobject");
    sphereObject->add<ComponentMaterial>().material = matDef;

    SceneObject *lightObject = scene.addSceneObject("light", Transform({4, 1.5, 0}));
    lightObject->add<ComponentLight>().light = instanceLights.get("defaultPointLight");

    scene.environmentIntensity() = 1.0F;
}

void performRenderSequence(VulkanEngine &engine, Scene &scene)
{
    engine.renderer().rendererPathTracing().renderInfo().samples = 64;
    engine.renderer().rendererPathTracing().renderInfo().batchSize = 64;
    engine.renderer().rendererPathTracing().renderInfo().fileType = FileType::PNG;
    engine.renderer().rendererPathTracing().renderInfo().denoise = true;
    engine.renderer().rendererPathTracing().renderInfo().writeAllFiles = false;

    uint32_t i = 0;
    float height = 1.0F;
    float radius = 10.0F;
    float angleStep = glm::radians(45.0F);
    for (float a = 0; a < 2 * M_PI; a += angleStep) {
        engine.renderer().rendererPathTracing().renderInfo().filename = std::to_string(i++);

        scene.camera()->transform().position() = glm::vec3(radius * sin(a), height, radius * cos(a));
        scene.camera()->transform().setRotationEuler(0, a, 0);

        engine.renderer().rendererPathTracing().render(scene);
    }
}

int main(int argc, char **argv)
{
    VulkanEngine engine("offlinerender");
    engine.initResources();

    Scene &scene = engine.scene();
    createSceneBallOnPlane(scene);

    scene.updateSceneGraph();

    performRenderSequence(engine, scene);

    engine.releaseResources();

    return 1;
}