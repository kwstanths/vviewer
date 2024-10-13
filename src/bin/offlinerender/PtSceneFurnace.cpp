#include "PtSceneFurnace.hpp"

#include "SceneUtils.hpp"

PtSceneFurnace::PtSceneFurnace(vengine::Engine &engine)
    : PtScene(engine){};

bool PtSceneFurnace::create()
{
    auto camera = std::make_shared<vengine::PerspectiveCamera>();
    camera->transform().position() = glm::vec3(0, 0, 2);
    camera->transform().setRotation(glm::quat(glm::vec3(0, 0, 0)));
    camera->fov() = 60.0f;
    camera->lensRadius() = 0.0F;
    camera->focalDistance() = 0.0F;
    scene().camera() = camera;

    vengine::SceneObject *so = scene().addSceneObject("sphere", vengine::Transform({0, 0, 0}, {1, 1, 1}));

    auto &instanceModels = vengine::AssetManager::getInstance().modelsMap();
    auto sphere = instanceModels.get("assets/models/uvsphere.obj");
    so->add<vengine::ComponentMesh>().setMesh(sphere->mesh("defaultobject"));

    auto material = engine().materials().createMaterial<vengine::MaterialLambert>(vengine::AssetInfo("material"));
    material->albedo() = glm::vec4(0.6, 0.6, 0.6, 1);
    so->add<vengine::ComponentMaterial>().setMaterial(material);

    scene().environmentIntensity() = 1.0F;
    scene().environmentType() = vengine::EnvironmentType::SOLID_COLOR;
    scene().backgroundColor() = glm::vec3(1, 1, 1);
    scene().update();
    return true;
}

bool PtSceneFurnace::render()
{
    engine().renderer().rendererPathTracing().renderInfo().samples = 2048;
    engine().renderer().rendererPathTracing().renderInfo().batchSize = 64;
    engine().renderer().rendererPathTracing().renderInfo().fileType = vengine::FileType::HDR;
    engine().renderer().rendererPathTracing().renderInfo().denoise = false;
    engine().renderer().rendererPathTracing().renderInfo().writeAllFiles = false;
    engine().renderer().rendererPathTracing().renderInfo().filename = "test";
    engine().renderer().rendererPathTracing().render();

    return true;
}