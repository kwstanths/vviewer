#include "PtSceneSharedComponents.hpp"

#include "SceneUtils.hpp"

PtSceneSharedComponents::PtSceneSharedComponents(vengine::Engine &engine)
    : PtScene(engine){};

bool PtSceneSharedComponents::create()
{
    auto camera = std::make_shared<vengine::PerspectiveCamera>();
    camera->fov() = 60.0f;
    camera->transform().position() = glm::vec3(0, 20, 50);
    camera->transform().setRotation(glm::quat(glm::vec3(glm::radians(-15.F), glm::radians(90.F), 0)));
    scene().camera() = camera;

    auto &instanceModels = vengine::AssetManager::getInstance().modelsMap();
    auto &instanceMaterials = vengine::AssetManager::getInstance().materialsMap();

    auto matDef = instanceMaterials.get("defaultMaterial");
    auto cube = instanceModels.get("assets/models/cube.obj");

    vengine::SceneObject *root = scene().addSceneObject("root", nullptr, vengine::Transform());

    vengine::ComponentManager &instance = vengine::ComponentManager::getInstance();
    vengine::ComponentMesh *meshComponent = instance.create<vengine::ComponentMesh, vengine::ComponentOwnerShared>();
    vengine::ComponentMaterial *materialComponent = instance.create<vengine::ComponentMaterial, vengine::ComponentOwnerShared>();

    int32_t size = 300;
    for (int32_t i = -size; i < size; i += 3) {
        for (int32_t j = -size; j < size; j += 3) {
            vengine::SceneObject *so = scene().addSceneObject(
                "cube" + std::to_string(i) + std::to_string(j), nullptr, vengine::Transform({i, 0, j}, {1, 1, 1}));
            so->add_shared<vengine::ComponentMesh>(meshComponent);
            so->add_shared<vengine::ComponentMaterial>(materialComponent);
        }
    }
    meshComponent->setMesh(cube->mesh("Cube"));
    materialComponent->setMaterial(matDef);

    {
        vengine::SceneObject *so = scene().addSceneObject(
            "directionalLight", nullptr, vengine::Transform({0, 2, 0}, {1, 1, 1}, {glm::radians(45.F), glm::radians(90.F), 0}));

        so->add<vengine::ComponentLight>().setLight(
            vengine::AssetManager::getInstance().lightsMap().get("defaultDirectionalLightSun"));
    }

    scene().environmentType() = vengine::EnvironmentType::SOLID_COLOR;
    scene().environmentIntensity() = 0.0F;

    scene().update();
    return true;
}

bool PtSceneSharedComponents::render()
{
    /* Performs a render sequence */

    engine().renderer().rendererPathTracing().renderInfo().samples = 512;
    engine().renderer().rendererPathTracing().renderInfo().batchSize = 32;
    engine().renderer().rendererPathTracing().renderInfo().fileType = vengine::FileType::HDR;
    engine().renderer().rendererPathTracing().renderInfo().denoise = false;
    engine().renderer().rendererPathTracing().renderInfo().writeAllFiles = false;
    engine().renderer().rendererPathTracing().renderInfo().filename = "test";
    engine().renderer().rendererPathTracing().render();

    return true;
}