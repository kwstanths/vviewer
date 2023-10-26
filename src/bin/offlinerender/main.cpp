#include <iostream>

#include "vengine/vulkan/VulkanEngine.hpp"

using namespace vengine;

void createBallOnPlanceScene(Scene &scene)
{
    auto camera = std::make_shared<PerspectiveCamera>();
    camera->fov() = 60.0f;
    camera->transform().position() = glm::vec3(0, 3, 10);
    camera->transform().setRotation(glm::quat(glm::vec3(glm::radians(-15.F), 0, 0)));
    scene.camera() = camera;

    auto &instanceModels = AssetManager::getInstance().modelsMap();
    auto &instanceMaterials = AssetManager::getInstance().materialsMap();
    auto &instanceLightMaterials = AssetManager::getInstance().lightMaterialsMap();

    auto matDef = instanceMaterials.get("defaultMaterial");
    auto plane = instanceModels.get("assets/models/plane.obj");
    auto sphere = instanceModels.get("assets/models/uvsphere.obj");

    std::shared_ptr<SceneObject> planeObject = scene.addSceneObject("plane", Transform({0, -3, 0}, {10, 10, 10}));
    planeObject->add<ComponentMesh>().mesh = plane->mesh("Plane");
    planeObject->add<ComponentMaterial>().material = matDef;

    std::shared_ptr<SceneObject> sphereObject = scene.addSceneObject("sphere", Transform({0, 0, 0}, {3, 3, 3}));
    sphereObject->add<ComponentMesh>().mesh = sphere->mesh("defaultobject");
    sphereObject->add<ComponentMaterial>().material = matDef;
}

int main(int argc, char **argv)
{
    VulkanEngine engine("offlinerender");
    engine.initResources();

    Scene &scene = engine.scene();
    createBallOnPlanceScene(scene);

    scene.updateSceneGraph();

    engine.renderer().rendererRayTracing().renderInfo().samples = 256;
    engine.renderer().rendererRayTracing().render(scene);

    engine.releaseResources();

    return 1;
}