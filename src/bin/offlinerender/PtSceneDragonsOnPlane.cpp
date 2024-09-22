#include "PtSceneDragonsOnPlane.hpp"

#include <filesystem>

#include "SceneUtils.hpp"

PtSceneDragonsOnPlane::PtSceneDragonsOnPlane(vengine::Engine &engine)
    : PtScene(engine){};

bool PtSceneDragonsOnPlane::create()
{
    std::string assetName = "assets/models/dragon/scene.gltf";
    if (!std::filesystem::exists(assetName)) {
        debug_tools::ConsoleFatal("Asset: " + assetName + " is missing");
        exit(-1);
    }
    engine().importModel(vengine::AssetInfo(assetName));

    auto camera = std::make_shared<vengine::PerspectiveCamera>();
    camera->transform().position() = glm::vec3(84, 6, 13);
    camera->transform().setRotation(glm::quat(glm::vec3(glm::radians(-15.F), glm::radians(42.F), 0)));
    camera->fov() = 60.0f;
    camera->lensRadius() = 0.4F;
    camera->focalDistance() = 20.F;
    scene().camera() = camera;

    auto floorMat = engine().materials().createMaterial<vengine::MaterialPBRStandard>(vengine::AssetInfo("floorMat"));
    floorMat->albedo() = glm::vec4(0.7, 0.7, 0.7, 1);
    floorMat->metallic() = 0.3F;
    floorMat->roughness() = 0.07F;
    auto wallMat = engine().materials().createMaterial<vengine::MaterialPBRStandard>(vengine::AssetInfo("wallMat"));
    wallMat->albedo() = glm::vec4(0.7, 0.7, 0.7, 1);
    wallMat->metallic() = 0.3F;
    wallMat->roughness() = 0.3F;

    addModel3D(scene(), nullptr, "plane", "assets/models/plane.obj", vengine::Transform({0, 0, 0}, {400, 400, 400}), "floorMat");
    addModel3D(scene(),
               nullptr,
               "plane",
               "assets/models/plane.obj",
               vengine::Transform({0, 0, -200}, {400, 400, 400}, {glm::radians(90.F), 0, 0}),
               "wallMat");
    addModel3D(scene(),
               nullptr,
               "plane",
               "assets/models/plane.obj",
               vengine::Transform({-200, 0, 0}, {400, 400, 400}, {0, 0, glm::radians(90.F)}),
               "wallMat");
    addModel3D(scene(),
               nullptr,
               "plane",
               "assets/models/plane.obj",
               vengine::Transform({-80, 0, -80}, {400, 400, 400}, {glm::radians(90.F), glm::radians(45.F), 0}),
               "wallMat");
    for (int32_t i = -90; i <= 90; i += 10) {
        addModel3D(scene(),
                   nullptr,
                   "dragon" + std::to_string(i),
                   "assets/models/dragon/scene.gltf",
                   vengine::Transform({i, -2.7, 0}, {50, 50, 50}, {0, 0, 0}));
    }

    auto envMap = engine().importEnvironmentMap(vengine::AssetInfo("assets/HDR/kloppenheim_07_puresky_4k.hdr"));
    scene().skyboxMaterial()->setMap(envMap);

    scene().environmentIntensity() = 2.0F;

    scene().update();
    return true;
}

bool PtSceneDragonsOnPlane::render()
{
    engine().renderer().rendererPathTracing().renderInfo().width = 1600;
    engine().renderer().rendererPathTracing().renderInfo().height = 1080;
    engine().renderer().rendererPathTracing().renderInfo().samples = 64;
    engine().renderer().rendererPathTracing().renderInfo().batchSize = 64;
    engine().renderer().rendererPathTracing().renderInfo().fileType = vengine::FileType::HDR;
    engine().renderer().rendererPathTracing().renderInfo().denoise = true;
    engine().renderer().rendererPathTracing().renderInfo().writeAllFiles = false;
    engine().renderer().rendererPathTracing().renderInfo().filename = "test";

    engine().renderer().rendererPathTracing().render();

    return true;
}