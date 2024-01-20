#include "PtSceneBreakfastRoom.hpp"

#include <filesystem>

#include "SceneUtils.hpp"
#include "debug_tools/Console.hpp"

PtSceneBreakfastRoom::PtSceneBreakfastRoom(vengine::Engine &engine)
    : PtScene(engine){};

bool PtSceneBreakfastRoom::create()
{
    std::string assetName = "assets/models/breakfast_room/breakfast_room.obj";
    if (!std::filesystem::exists(assetName)) {
        debug_tools::ConsoleFatal("Asset: " + assetName +
                                  " is missing. Download asset: "
                                  "https://casual-effects.com/g3d/data10/research/model/breakfast_room/breakfast_room.zip inside "
                                  "assets/models/ folder");
        exit(-1);
    }

    engine().importModel(vengine::AssetInfo(assetName));

    auto camera = std::make_shared<vengine::PerspectiveCamera>();
    camera->transform().position() = glm::vec3(-0.8, 2.5, 5);
    camera->transform().setRotation(glm::quat(glm::vec3(glm::radians(-10.F), 0, 0)));
    camera->fov() = 60.0f;
    camera->lensRadius() = 0.0F;
    camera->focalDistance() = 7.0F;
    scene().camera() = camera;

    addModel3D(scene(), nullptr, "breakfastroom", assetName, vengine::Transform());

    /* Get default directional light */
    auto &instanceLights = vengine::AssetManager::getInstance().lightsMap();
    auto dirLight = static_cast<vengine::DirectionalLight *>(instanceLights.get("defaultDirectionalLight"));

    vengine::SceneObject *lightObject =
        scene().addSceneObject("light", vengine::Transform({0, 0, 0}, {1, 1, 1}, {glm::radians(45.F), glm::radians(-90.F), 0}));
    lightObject->add<vengine::ComponentLight>().light = dirLight;
    dirLight->color().a = 20.F;

    scene().environmentIntensity() = 0.0F;

    scene().updateSceneGraph();
    return true;
}

bool PtSceneBreakfastRoom::render()
{
    engine().renderer().rendererPathTracing().renderInfo().width = 1600;
    engine().renderer().rendererPathTracing().renderInfo().height = 1024;
    engine().renderer().rendererPathTracing().renderInfo().samples = 64;
    engine().renderer().rendererPathTracing().renderInfo().batchSize = 16;
    engine().renderer().rendererPathTracing().renderInfo().fileType = vengine::FileType::HDR;
    engine().renderer().rendererPathTracing().renderInfo().denoise = true;
    engine().renderer().rendererPathTracing().renderInfo().writeAllFiles = false;
    engine().renderer().rendererPathTracing().renderInfo().filename = "test";
    engine().renderer().rendererPathTracing().render(scene());

    return true;
}