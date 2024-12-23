#include "RenderTests.hpp"

VulkanEngine *RenderPTTest::mEngine = nullptr;

float CompareImages(const std::string &filename1, const std::string &filename2, int channels)
{
    int size_x, size_y;
    float *file1 = stbi_loadf(filename1.c_str(), &size_x, &size_y, &channels, channels);
    float *file2 = stbi_loadf(filename2.c_str(), &size_x, &size_y, &channels, channels);

    if (file1 == nullptr || file2 == nullptr) {
        std::cout << "Can't open file" << std::endl;
        return 1000;
    }

    int size = size_x * size_y * channels;
    double MSE = 0;
    for (size_t i = 0; i < size; i++) {
        MSE += std::pow(file1[i] - file2[i], 2);
    }

    MSE = MSE / (double)size;
    return MSE;
}

void PrepareBallOnPlaneScene(Engine &engine)
{
    Scene &scene = engine.scene();

    auto camera = std::make_shared<vengine::PerspectiveCamera>();
    camera->transform().position() = glm::vec3(0, 1, 4);
    camera->transform().setRotation(glm::quat(glm::vec3(glm::radians(0.F), 0, 0)));
    scene.camera() = camera;

    auto &instanceModels = vengine::AssetManager::getInstance().modelsMap();
    auto sphereModel = instanceModels.get("assets/models/uvsphere.obj");
    Mesh *sphereMesh = sphereModel->mesh("defaultobject");
    auto planeModel = instanceModels.get("assets/models/plane.obj");
    Mesh *planeMesh = planeModel->mesh("Plane");
    auto cubeModel = instanceModels.get("assets/models/cube.obj");
    Mesh *cubeMesh = cubeModel->mesh("Cube");

    auto materialPBR = engine.materials().createMaterial<vengine::MaterialPBRStandard>(vengine::AssetInfo("pbrmaterial"));
    auto materialLambert = engine.materials().createMaterial<vengine::MaterialLambert>(vengine::AssetInfo("lambertmaterial"));

    {
        vengine::SceneObject *so = scene.addSceneObject("sphere", vengine::Transform({0, 1, 0}, {1, 1, 1}));
        so->add<vengine::ComponentMesh>().setMesh(sphereMesh);
        so->add<vengine::ComponentMaterial>().setMaterial(materialPBR);
    }

    {
        vengine::SceneObject *so = scene.addSceneObject("plane", vengine::Transform({0, 0, 0}, {10, 10, 10}));
        so->add<vengine::ComponentMesh>().setMesh(planeMesh);
        so->add<vengine::ComponentMaterial>().setMaterial(materialLambert);
    }

    scene.environmentIntensity() = 1.0F;
    scene.update();
}

TEST_F(RenderPTTest, EnvironmentMap)
{
    Engine &engine = *mEngine;

    PrepareBallOnPlaneScene(engine);

    engine.renderer().rendererPathTracing().renderInfo().filename = "EnvironmentMap_test";
    engine.renderer().rendererPathTracing().renderInfo().width = 256;
    engine.renderer().rendererPathTracing().renderInfo().height = 256;
    engine.renderer().rendererPathTracing().renderInfo().samples = 1024;
    engine.renderer().rendererPathTracing().renderInfo().batchSize = 64;
    engine.renderer().rendererPathTracing().renderInfo().fileType = vengine::FileType::HDR;
    engine.renderer().rendererPathTracing().renderInfo().denoise = false;
    engine.renderer().rendererPathTracing().renderInfo().writeAllFiles = false;
    engine.renderer().rendererPathTracing().render();

    float diff = CompareImages("EnvironmentMap_test.hdr", "assets/unittests/EnvironmentMap_ref.hdr", 3);
    EXPECT_LE(diff, 0.0001);
}

TEST_F(RenderPTTest, EnvironmentMapPBR)
{
    Engine &engine = *mEngine;
    Scene &scene = engine.scene();

    auto camera = std::make_shared<vengine::PerspectiveCamera>();
    camera->transform().position() = glm::vec3(0, 0, 3);
    camera->transform().setRotation(glm::quat(glm::vec3(glm::radians(0.F), 0, 0)));
    scene.camera() = camera;

    auto &instanceModels = vengine::AssetManager::getInstance().modelsMap();
    auto sphereModel = instanceModels.get("assets/models/uvsphere.obj");
    Mesh *sphereMesh = sphereModel->mesh("defaultobject");

    auto materialPBR = engine.materials().createMaterial<vengine::MaterialPBRStandard>(vengine::AssetInfo("pbrmaterial"));
    {
        vengine::SceneObject *so = scene.addSceneObject("sphere", vengine::Transform({0, 0, 0}, {1, 1, 1}));
        so->add<vengine::ComponentMesh>().setMesh(sphereMesh);
        so->add<vengine::ComponentMaterial>().setMaterial(materialPBR);
    }

    scene.environmentIntensity() = 1.0F;
    scene.update();

    engine.renderer().rendererPathTracing().renderInfo().width = 256;
    engine.renderer().rendererPathTracing().renderInfo().height = 256;
    engine.renderer().rendererPathTracing().renderInfo().samples = 1024;
    engine.renderer().rendererPathTracing().renderInfo().batchSize = 64;
    engine.renderer().rendererPathTracing().renderInfo().fileType = vengine::FileType::HDR;
    engine.renderer().rendererPathTracing().renderInfo().denoise = false;
    engine.renderer().rendererPathTracing().renderInfo().writeAllFiles = false;
    float diff;

    materialPBR->roughness() = 0.F;
    materialPBR->metallic() = 0.F;
    engine.renderer().rendererPathTracing().renderInfo().filename = "EnvironmentMapPBR00_test";
    engine.renderer().rendererPathTracing().render();
    diff = CompareImages("EnvironmentMapPBR00_test.hdr", "assets/unittests/EnvironmentMapPBR00_ref.hdr", 3);
    EXPECT_LE(diff, 0.00001);

    materialPBR->roughness() = 0.F;
    materialPBR->metallic() = 1.F;
    engine.renderer().rendererPathTracing().renderInfo().filename = "EnvironmentMapPBR01_test";
    engine.renderer().rendererPathTracing().render();
    diff = CompareImages("EnvironmentMapPBR01_test.hdr", "assets/unittests/EnvironmentMapPBR01_ref.hdr", 3);
    EXPECT_LE(diff, 0.00001);

    materialPBR->roughness() = 1.F;
    materialPBR->metallic() = 0.F;
    engine.renderer().rendererPathTracing().renderInfo().filename = "EnvironmentMapPBR10_test";
    engine.renderer().rendererPathTracing().render();
    diff = CompareImages("EnvironmentMapPBR10_test.hdr", "assets/unittests/EnvironmentMapPBR10_ref.hdr", 3);
    EXPECT_LE(diff, 0.00005);

    materialPBR->roughness() = 1.F;
    materialPBR->metallic() = 1.F;
    engine.renderer().rendererPathTracing().renderInfo().filename = "EnvironmentMapPBR11_test";
    engine.renderer().rendererPathTracing().render();
    diff = CompareImages("EnvironmentMapPBR11_test.hdr", "assets/unittests/EnvironmentMapPBR11_ref.hdr", 3);
    EXPECT_LE(diff, 0.00001);
}

TEST_F(RenderPTTest, EnvironmentMapLambert)
{
    Engine &engine = *mEngine;
    Scene &scene = engine.scene();

    auto camera = std::make_shared<vengine::PerspectiveCamera>();
    camera->transform().position() = glm::vec3(0, 0, 3);
    camera->transform().setRotation(glm::quat(glm::vec3(glm::radians(0.F), 0, 0)));
    scene.camera() = camera;

    auto &instanceModels = vengine::AssetManager::getInstance().modelsMap();
    auto sphereModel = instanceModels.get("assets/models/uvsphere.obj");
    Mesh *sphereMesh = sphereModel->mesh("defaultobject");

    auto materialLambert = engine.materials().createMaterial<vengine::MaterialLambert>(vengine::AssetInfo("lambertmaterial"));
    {
        vengine::SceneObject *so = scene.addSceneObject("sphere", vengine::Transform({0, 0, 0}, {1, 1, 1}));
        so->add<vengine::ComponentMesh>().setMesh(sphereMesh);
        so->add<vengine::ComponentMaterial>().setMaterial(materialLambert);
    }

    scene.environmentIntensity() = 1.0F;
    scene.update();

    engine.renderer().rendererPathTracing().renderInfo().width = 256;
    engine.renderer().rendererPathTracing().renderInfo().height = 256;
    engine.renderer().rendererPathTracing().renderInfo().samples = 1024;
    engine.renderer().rendererPathTracing().renderInfo().batchSize = 64;
    engine.renderer().rendererPathTracing().renderInfo().fileType = vengine::FileType::HDR;
    engine.renderer().rendererPathTracing().renderInfo().denoise = false;
    engine.renderer().rendererPathTracing().renderInfo().writeAllFiles = false;
    float diff;

    materialLambert->albedo() = glm::vec4(0.1, 0.3, 0.5, 1.0);
    engine.renderer().rendererPathTracing().renderInfo().filename = "EnvironmentMapLambert_test";
    engine.renderer().rendererPathTracing().render();
    diff = CompareImages("EnvironmentMapLambert_test.hdr", "assets/unittests/EnvironmentMapLambert_ref.hdr", 3);
    EXPECT_LE(diff, 0.000005);
}

TEST_F(RenderPTTest, EnvironmentMapVolume)
{
    Engine &engine = *mEngine;
    Scene &scene = engine.scene();

    auto camera = std::make_shared<vengine::PerspectiveCamera>();
    camera->transform().position() = glm::vec3(0, 0, 3);
    camera->transform().setRotation(glm::quat(glm::vec3(glm::radians(0.F), 0, 0)));
    scene.camera() = camera;

    auto &instanceModels = vengine::AssetManager::getInstance().modelsMap();
    auto sphereModel = instanceModels.get("assets/models/uvsphere.obj");
    Mesh *sphereMesh = sphereModel->mesh("defaultobject");
    auto cubeModel = instanceModels.get("assets/models/cube.obj");
    Mesh *cubeMesh = cubeModel->mesh("Cube");

    auto &instanceMaterials = vengine::AssetManager::getInstance().materialsMap();

    auto materialVolume = engine.materials().createMaterial<vengine::MaterialVolume>(vengine::AssetInfo("volumematerial"));
    {
        vengine::SceneObject *so = scene.addSceneObject("sphere", vengine::Transform({0, 0, 0}, {1, 1, 1}));
        so->add<vengine::ComponentMesh>().setMesh(sphereMesh);
        so->add<vengine::ComponentMaterial>().setMaterial(materialVolume);
    }
    {
        vengine::SceneObject *so = scene.addSceneObject("cube", vengine::Transform({-1, -1, -1}, {1, 1, 1}));
        so->add<vengine::ComponentMesh>().setMesh(cubeMesh);
        so->add<vengine::ComponentMaterial>().setMaterial(instanceMaterials.get("defaultMaterial"));
    }

    scene.environmentIntensity() = 1.0F;
    scene.update();

    engine.renderer().rendererPathTracing().renderInfo().width = 256;
    engine.renderer().rendererPathTracing().renderInfo().height = 256;
    engine.renderer().rendererPathTracing().renderInfo().samples = 1024;
    engine.renderer().rendererPathTracing().renderInfo().batchSize = 64;
    engine.renderer().rendererPathTracing().renderInfo().fileType = vengine::FileType::HDR;
    engine.renderer().rendererPathTracing().renderInfo().denoise = false;
    engine.renderer().rendererPathTracing().renderInfo().writeAllFiles = false;
    float diff;

    materialVolume->sigmaA() = glm::vec4(0.2F, 0.4F, 0.8F, 0.F);
    materialVolume->sigmaS() = glm::vec4(0.8F, 0.4F, 0.2F, 0.F);

    materialVolume->g() = 0.0F;
    engine.renderer().rendererPathTracing().renderInfo().filename = "EnvironmentMapVolume0_test";
    engine.renderer().rendererPathTracing().render();
    diff = CompareImages("EnvironmentMapVolume0_test.hdr", "assets/unittests/EnvironmentMapVolume0_ref.hdr", 3);
    EXPECT_LE(diff, 0.00001);

    materialVolume->g() = -1.0F;
    engine.renderer().rendererPathTracing().renderInfo().filename = "EnvironmentMapVolume-1_test";
    engine.renderer().rendererPathTracing().render();
    diff = CompareImages("EnvironmentMapVolume-1_test.hdr", "assets/unittests/EnvironmentMapVolume-1_ref.hdr", 3);
    EXPECT_LE(diff, 0.000008);

    materialVolume->g() = 1.0F;
    engine.renderer().rendererPathTracing().renderInfo().filename = "EnvironmentMapVolume1_test";
    engine.renderer().rendererPathTracing().render();
    diff = CompareImages("EnvironmentMapVolume1_test.hdr", "assets/unittests/EnvironmentMapVolume1_ref.hdr", 3);
    EXPECT_LE(diff, 0.00001);
}