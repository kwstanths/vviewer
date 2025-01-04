#include "RenderTests.hpp"

#include "vengine/core/SceneUtils.hpp"

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
    return static_cast<float>(MSE);
}

void PrepareTwoBallsOnPlaneScene(Engine &engine)
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

    auto materialPBR = engine.materials().createMaterial<vengine::MaterialPBRStandard>(vengine::AssetInfo("pbrmaterial2"));
    auto materialLambert = engine.materials().createMaterial<vengine::MaterialLambert>(vengine::AssetInfo("lambertmaterial2"));
    auto materialFloor = engine.materials().createMaterial<vengine::MaterialLambert>(vengine::AssetInfo("lambertmaterial3"));

    materialPBR->albedo() = glm::vec4(0.7, 0.1, 0.1, 1.0);
    materialPBR->roughness() = 0.3F;
    materialLambert->albedo() = glm::vec4(0.1, 0.7, 0.1, 1.0);
    {
        vengine::SceneObject *so = scene.addSceneObject("sphere", vengine::Transform({-1, 1, 0}, {1, 1, 1}));
        so->add<vengine::ComponentMesh>().setMesh(sphereMesh);
        so->add<vengine::ComponentMaterial>().setMaterial(materialPBR);
    }

    {
        vengine::SceneObject *so = scene.addSceneObject("sphere", vengine::Transform({1, 1, 0}, {1, 1, 1}));
        so->add<vengine::ComponentMesh>().setMesh(sphereMesh);
        so->add<vengine::ComponentMaterial>().setMaterial(materialLambert);
    }

    {
        vengine::SceneObject *so = scene.addSceneObject("plane", vengine::Transform({0, 0, 0}, {10, 10, 10}));
        so->add<vengine::ComponentMesh>().setMesh(planeMesh);
        so->add<vengine::ComponentMaterial>().setMaterial(materialFloor);
    }

    scene.environmentIntensity() = 1.0F;
    scene.update();
}

TEST_F(RenderPTTest, FurnacePBR)
{
    Engine &engine = *mEngine;

    auto camera = std::make_shared<vengine::PerspectiveCamera>();
    camera->transform().position() = glm::vec3(0, 0, 2);
    camera->transform().setRotation(glm::quat(glm::vec3(0, 0, 0)));
    camera->fov() = 60.0f;
    camera->lensRadius() = 0.0F;
    camera->focalDistance() = 0.0F;
    engine.scene().camera() = camera;

    vengine::SceneObject *so = engine.scene().addSceneObject("sphere", vengine::Transform({0, 0, 0}, {1, 1, 1}));

    auto &instanceModels = vengine::AssetManager::getInstance().modelsMap();
    auto sphere = instanceModels.get("assets/models/uvsphere.obj");
    so->add<vengine::ComponentMesh>().setMesh(sphere->mesh("defaultobject"));

    auto material = engine.materials().createMaterial<vengine::MaterialPBRStandard>(vengine::AssetInfo("pbrmaterial3"));
    material->albedo() = glm::vec4(0.6, 0.6, 0.6, 1);
    so->add<vengine::ComponentMaterial>().setMaterial(material);

    engine.scene().environmentIntensity() = 1.0F;
    engine.scene().environmentType() = vengine::EnvironmentType::SOLID_COLOR;
    engine.scene().backgroundColor() = glm::vec3(1, 1, 1);
    engine.scene().update();

    engine.renderer().rendererPathTracing().renderInfo().filename = "FurnacePBR_test";
    engine.renderer().rendererPathTracing().renderInfo().width = 256;
    engine.renderer().rendererPathTracing().renderInfo().height = 256;
    engine.renderer().rendererPathTracing().renderInfo().samples = 2048;
    engine.renderer().rendererPathTracing().renderInfo().batchSize = 64;
    engine.renderer().rendererPathTracing().renderInfo().fileType = vengine::FileType::HDR;
    engine.renderer().rendererPathTracing().renderInfo().denoise = false;
    engine.renderer().rendererPathTracing().renderInfo().writeAllFiles = false;
    engine.renderer().rendererPathTracing().render();

    float diff = CompareImages("FurnacePBR_test.hdr", "assets/unittests/FurnacePBR_ref.hdr", 3);
    EXPECT_LE(diff, 0.000001);
}

TEST_F(RenderPTTest, FurnaceLambert)
{
    Engine &engine = *mEngine;

    auto camera = std::make_shared<vengine::PerspectiveCamera>();
    camera->transform().position() = glm::vec3(0, 0, 2);
    camera->transform().setRotation(glm::quat(glm::vec3(0, 0, 0)));
    camera->fov() = 60.0f;
    camera->lensRadius() = 0.0F;
    camera->focalDistance() = 0.0F;
    engine.scene().camera() = camera;

    vengine::SceneObject *so = engine.scene().addSceneObject("sphere", vengine::Transform({0, 0, 0}, {1, 1, 1}));

    auto &instanceModels = vengine::AssetManager::getInstance().modelsMap();
    auto sphere = instanceModels.get("assets/models/uvsphere.obj");
    so->add<vengine::ComponentMesh>().setMesh(sphere->mesh("defaultobject"));

    auto material = engine.materials().createMaterial<vengine::MaterialLambert>(vengine::AssetInfo("lambertmaterial4"));
    material->albedo() = glm::vec4(0.6, 0.6, 0.6, 1);
    so->add<vengine::ComponentMaterial>().setMaterial(material);

    engine.scene().environmentIntensity() = 1.0F;
    engine.scene().environmentType() = vengine::EnvironmentType::SOLID_COLOR;
    engine.scene().backgroundColor() = glm::vec3(1, 1, 1);
    engine.scene().update();

    engine.renderer().rendererPathTracing().renderInfo().filename = "FurnaceLambert_test";
    engine.renderer().rendererPathTracing().renderInfo().width = 256;
    engine.renderer().rendererPathTracing().renderInfo().height = 256;
    engine.renderer().rendererPathTracing().renderInfo().samples = 2048;
    engine.renderer().rendererPathTracing().renderInfo().batchSize = 64;
    engine.renderer().rendererPathTracing().renderInfo().fileType = vengine::FileType::HDR;
    engine.renderer().rendererPathTracing().renderInfo().denoise = false;
    engine.renderer().rendererPathTracing().renderInfo().writeAllFiles = false;
    engine.renderer().rendererPathTracing().render();

    float diff = CompareImages("FurnaceLambert_test.hdr", "assets/unittests/FurnaceLambert_ref.hdr", 3);
    EXPECT_LE(diff, 0.000001);
}

TEST_F(RenderPTTest, EnvironmentMap)
{
    Engine &engine = *mEngine;
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

    auto materialPBR = engine.materials().createMaterial<vengine::MaterialPBRStandard>(vengine::AssetInfo("pbrmaterial1"));
    auto materialLambert = engine.materials().createMaterial<vengine::MaterialLambert>(vengine::AssetInfo("lambertmaterial1"));

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

    scene.environmentType() = vengine::EnvironmentType::HDRI;
    scene.environmentIntensity() = 1.0F;
    scene.update();

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

    auto materialPBR = engine.materials().createMaterial<vengine::MaterialPBRStandard>(vengine::AssetInfo("pbrmaterial4"));
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

    auto materialLambert = engine.materials().createMaterial<vengine::MaterialLambert>(vengine::AssetInfo("lambertmaterial5"));
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

    auto materialVolume = engine.materials().createMaterial<vengine::MaterialVolume>(vengine::AssetInfo("volumematerial1"));
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

TEST_F(RenderPTTest, PointLight)
{
    Engine &engine = *mEngine;

    PrepareTwoBallsOnPlaneScene(engine);

    engine.scene().environmentType() = EnvironmentType::SOLID_COLOR;
    engine.scene().backgroundColor() = glm::vec3(0);
    {
        auto* pointLight = engine.scene().createLight(AssetInfo("Point light", "Point light"), LightType::POINT_LIGHT);
        static_cast<PointLight *>(pointLight)->color().a = 10;
        auto* so = engine.scene().addSceneObject("Point light", Transform({0, 4, 0}));
        so->add<ComponentLight>().setLight(pointLight);
    }
    engine.scene().update();

    engine.renderer().rendererPathTracing().renderInfo().filename = "PointLight_test";
    engine.renderer().rendererPathTracing().renderInfo().width = 256;
    engine.renderer().rendererPathTracing().renderInfo().height = 256;
    engine.renderer().rendererPathTracing().renderInfo().samples = 2048;
    engine.renderer().rendererPathTracing().renderInfo().batchSize = 64;
    engine.renderer().rendererPathTracing().renderInfo().fileType = vengine::FileType::HDR;
    engine.renderer().rendererPathTracing().renderInfo().denoise = false;
    engine.renderer().rendererPathTracing().renderInfo().writeAllFiles = false;
    engine.renderer().rendererPathTracing().render();

    float diff = CompareImages("PointLight_test.hdr", "assets/unittests/PointLight_ref.hdr", 3);
    EXPECT_LE(diff, 0.00000001);
}

TEST_F(RenderPTTest, DirectionalLight)
{
    Engine &engine = *mEngine;

    PrepareTwoBallsOnPlaneScene(engine);

    engine.scene().environmentType() = EnvironmentType::SOLID_COLOR;
    engine.scene().backgroundColor() = glm::vec3(0);
    {
        auto *directionalLight = engine.scene().createLight(AssetInfo("Directional light", "Directional light"), LightType::DIRECTIONAL_LIGHT);
        static_cast<DirectionalLight *>(directionalLight)->color().a = 1;
        auto *so = engine.scene().addSceneObject("Directional light",
                                                 Transform({0, 0, 0}, {1, 1, 1}, {glm::radians(45.F), glm::radians(90.F), 0}));
        so->add<ComponentLight>().setLight(directionalLight);
    }
    engine.scene().update();

    engine.renderer().rendererPathTracing().renderInfo().filename = "DirectionalLight_test";
    engine.renderer().rendererPathTracing().renderInfo().width = 256;
    engine.renderer().rendererPathTracing().renderInfo().height = 256;
    engine.renderer().rendererPathTracing().renderInfo().samples = 2048;
    engine.renderer().rendererPathTracing().renderInfo().batchSize = 64;
    engine.renderer().rendererPathTracing().renderInfo().fileType = vengine::FileType::HDR;
    engine.renderer().rendererPathTracing().renderInfo().denoise = false;
    engine.renderer().rendererPathTracing().renderInfo().writeAllFiles = false;
    engine.renderer().rendererPathTracing().render();

    float diff = CompareImages("DirectionalLight_test.hdr", "assets/unittests/DirectionalLight_ref.hdr", 3);
    EXPECT_LE(diff, 0.00000001);
}

TEST_F(RenderPTTest, MeshLight)
{
    Engine &engine = *mEngine;

    PrepareTwoBallsOnPlaneScene(engine);

    engine.scene().environmentType() = EnvironmentType::SOLID_COLOR;
    engine.scene().backgroundColor() = glm::vec3(0);
    {
        auto *so = engine.scene().addSceneObject("Mesh light",
                                                 Transform({0, 2, 0}, {0.4, 0.4, 0.4}, {glm::radians(90.F), glm::radians(0.F), 0}));

        auto &instanceModels = vengine::AssetManager::getInstance().modelsMap();
        auto planeModel = instanceModels.get("assets/models/plane.obj");
        Mesh *planeMesh = planeModel->mesh("Plane");
        so->add<ComponentMesh>().setMesh(planeMesh);
        
        auto materialPBREmissive =
            engine.materials().createMaterial<vengine::MaterialPBRStandard>(vengine::AssetInfo("emissivepbrmaterial"));
        materialPBREmissive->emissive() = glm::vec4(0.3, 0.3, 0.7, 15.0);
        so->add<ComponentMaterial>().setMaterial(materialPBREmissive);

        engine.scene().camera()->transform().position() = glm::vec3(0, 3, 5);
        engine.scene().camera()->transform().setRotation(glm::quat(glm::vec3(glm::radians(-20.F), 0, 0)));
    }
    engine.scene().update();

    engine.renderer().rendererPathTracing().renderInfo().filename = "MeshLight_test";
    engine.renderer().rendererPathTracing().renderInfo().width = 256;
    engine.renderer().rendererPathTracing().renderInfo().height = 256;
    engine.renderer().rendererPathTracing().renderInfo().samples = 2048;
    engine.renderer().rendererPathTracing().renderInfo().batchSize = 64;
    engine.renderer().rendererPathTracing().renderInfo().fileType = vengine::FileType::HDR;
    engine.renderer().rendererPathTracing().renderInfo().denoise = false;
    engine.renderer().rendererPathTracing().renderInfo().writeAllFiles = false;
    engine.renderer().rendererPathTracing().render();

    float diff = CompareImages("MeshLight_test.hdr", "assets/unittests/MeshLight_ref.hdr", 3);
    EXPECT_LE(diff, 0.00000001);
}

TEST_F(RenderPTTest, Transparency)
{
    Engine &engine = *mEngine;
    Scene &scene = engine.scene();

    auto camera = std::make_shared<vengine::PerspectiveCamera>();
    camera->transform().position() = glm::vec3(0, 2, 4);
    camera->transform().setRotation(glm::quat(glm::vec3(glm::radians(-20.F), 0, 0)));
    scene.camera() = camera;

    auto &instanceModels = vengine::AssetManager::getInstance().modelsMap();
    auto sphereModel = instanceModels.get("assets/models/uvsphere.obj");
    Mesh *sphereMesh = sphereModel->mesh("defaultobject");
    auto planeModel = instanceModels.get("assets/models/plane.obj");
    Mesh *planeMesh = planeModel->mesh("Plane");

    {
        auto materialPBR1 = engine.materials().createMaterial<vengine::MaterialPBRStandard>(vengine::AssetInfo("pbrmaterial5"));
        materialPBR1->setTransparent(true);
        materialPBR1->albedo() = glm::vec4(0.7, 0.7, 0.7, 0.5);
        vengine::SceneObject *so = scene.addSceneObject("sphere", vengine::Transform({0, 0, 0}, {1, 1, 1}));
        so->add<vengine::ComponentMesh>().setMesh(sphereMesh);
        so->add<vengine::ComponentMaterial>().setMaterial(materialPBR1);
    }
    {
        auto materialPBR2 = engine.materials().createMaterial<vengine::MaterialPBRStandard>(vengine::AssetInfo("pbrmaterial6"));
        auto* tex = engine.textures().createTexture(AssetInfo("checkerboard", "assets/textures/checkerboard.png"));
        materialPBR2->setAlphaTexture(tex);
        materialPBR2->setTransparent(true);

        vengine::SceneObject *so = scene.addSceneObject("plane", vengine::Transform({0, 0, 0}, {5, 5, 5}));
        so->add<vengine::ComponentMesh>().setMesh(planeMesh);
        so->add<vengine::ComponentMaterial>().setMaterial(materialPBR2);
    }

    engine.scene().environmentType() = EnvironmentType::HDRI;
    scene.environmentIntensity() = 1.0F;
    scene.update();

    engine.renderer().rendererPathTracing().renderInfo().filename = "Transparency_test";
    engine.renderer().rendererPathTracing().renderInfo().width = 256;
    engine.renderer().rendererPathTracing().renderInfo().height = 256;
    engine.renderer().rendererPathTracing().renderInfo().samples = 2048;
    engine.renderer().rendererPathTracing().renderInfo().batchSize = 64;
    engine.renderer().rendererPathTracing().renderInfo().fileType = vengine::FileType::HDR;
    engine.renderer().rendererPathTracing().renderInfo().denoise = false;
    engine.renderer().rendererPathTracing().renderInfo().writeAllFiles = false;
    engine.renderer().rendererPathTracing().render();
    
    float diff = CompareImages("Transparency_test.hdr", "assets/unittests/Transparency_ref.hdr", 3);
    EXPECT_LE(diff, 0.000001);
}

TEST_F(RenderPTTest, NormalMap)
{
    Engine &engine = *mEngine;
    Scene &scene = engine.scene();

    auto camera = std::make_shared<vengine::PerspectiveCamera>();
    camera->transform().position() = glm::vec3(0, 3, 8);
    camera->transform().setRotation(glm::quat(glm::vec3(glm::radians(-20.F), 0, 0)));
    scene.camera() = camera;

    auto &instanceModels = vengine::AssetManager::getInstance().modelsMap();
    auto sphereModel = instanceModels.get("assets/models/uvsphere.obj");
    Mesh *sphereMesh = sphereModel->mesh("defaultobject");
    auto planeModel = instanceModels.get("assets/models/plane.obj");
    Mesh *planeMesh = planeModel->mesh("Plane");

    {
        auto materialPBR = engine.materials().createMaterial<vengine::MaterialPBRStandard>(vengine::AssetInfo("pbrmaterial7"));
        auto *tex = engine.textures().createTexture(AssetInfo("normal", "assets/textures/normal.png"), ColorSpace::LINEAR);
        materialPBR->setNormalTexture(tex);

        vengine::SceneObject *so = scene.addSceneObject("plane", vengine::Transform({0, 0, 0}, {5, 5, 5}));
        so->add<vengine::ComponentMesh>().setMesh(planeMesh);
        so->add<vengine::ComponentMaterial>().setMaterial(materialPBR);
    }

    engine.scene().environmentType() = EnvironmentType::HDRI;
    scene.environmentIntensity() = 1.0F;
    scene.update();

    engine.renderer().rendererPathTracing().renderInfo().filename = "NormalMap_test";
    engine.renderer().rendererPathTracing().renderInfo().width = 256;
    engine.renderer().rendererPathTracing().renderInfo().height = 256;
    engine.renderer().rendererPathTracing().renderInfo().samples = 2048;
    engine.renderer().rendererPathTracing().renderInfo().batchSize = 64;
    engine.renderer().rendererPathTracing().renderInfo().fileType = vengine::FileType::HDR;
    engine.renderer().rendererPathTracing().renderInfo().denoise = false;
    engine.renderer().rendererPathTracing().renderInfo().writeAllFiles = false;
    engine.renderer().rendererPathTracing().render();

    float diff = CompareImages("NormalMap_test.hdr", "assets/unittests/NormalMap_ref.hdr", 3);
    EXPECT_LE(diff, 0.000001);
}

TEST_F(RenderPTTest, GLTF)
{
    Engine &engine = *mEngine;
    Scene &scene = engine.scene();

    auto camera = std::make_shared<vengine::PerspectiveCamera>();
    camera->transform().position() = glm::vec3(0, 0.5, 2);
    camera->transform().setRotation(glm::quat(glm::vec3(glm::radians(-20.F), 0, 0)));
    scene.camera() = camera;

    auto &instanceModels = vengine::AssetManager::getInstance().modelsMap();
    auto sphereModel = instanceModels.get("assets/models/uvsphere.obj");
    Mesh *sphereMesh = sphereModel->mesh("defaultobject");
    auto planeModel = instanceModels.get("assets/models/plane.obj");
    Mesh *planeMesh = planeModel->mesh("Plane");

    auto &instanceMaterials = vengine::AssetManager::getInstance().materialsMap();
    {
        vengine::SceneObject *so = scene.addSceneObject("plane", vengine::Transform({0, -1, 0}, {5, 5, 5}));
        so->add<vengine::ComponentMesh>().setMesh(planeMesh);
        so->add<vengine::ComponentMaterial>().setMaterial(instanceMaterials.get("defaultMaterial"));
    }

    std::string assetName = "assets/models/DamagedHelmet.gltf";
    engine.importModel(AssetInfo(assetName), true);
    addModel3D(scene, nullptr, assetName, std::nullopt, std::nullopt);

    engine.scene().environmentType() = EnvironmentType::HDRI;
    scene.environmentIntensity() = 1.0F;
    scene.update();

    engine.renderer().rendererPathTracing().renderInfo().filename = "GLTF_test";
    engine.renderer().rendererPathTracing().renderInfo().width = 256;
    engine.renderer().rendererPathTracing().renderInfo().height = 256;
    engine.renderer().rendererPathTracing().renderInfo().samples = 2048;
    engine.renderer().rendererPathTracing().renderInfo().batchSize = 64;
    engine.renderer().rendererPathTracing().renderInfo().fileType = vengine::FileType::HDR;
    engine.renderer().rendererPathTracing().renderInfo().denoise = false;
    engine.renderer().rendererPathTracing().renderInfo().writeAllFiles = false;
    engine.renderer().rendererPathTracing().render();

    float diff = CompareImages("GLTF_test.hdr", "assets/unittests/GLTF_ref.hdr", 3);
    EXPECT_LE(diff, 0.000001);
}

TEST_F(RenderPTTest, Hierarchy)
{
    Engine &engine = *mEngine;
    Scene &scene = engine.scene();

    auto camera = std::make_shared<vengine::PerspectiveCamera>();
    camera->transform().position() = glm::vec3(0, 3, 12);
    camera->transform().setRotation(glm::quat(glm::vec3(glm::radians(-20.F), 0, 0)));
    scene.camera() = camera;

    auto &instanceModels = vengine::AssetManager::getInstance().modelsMap();
    auto sphereModel = instanceModels.get("assets/models/uvsphere.obj");
    Mesh *sphereMesh = sphereModel->mesh("defaultobject");
    auto planeModel = instanceModels.get("assets/models/plane.obj");
    Mesh *planeMesh = planeModel->mesh("Plane");
    auto cubeModel = instanceModels.get("assets/models/cube.obj");
    Mesh *cubeMesh = cubeModel->mesh("Cube");

    auto &instanceMaterials = vengine::AssetManager::getInstance().materialsMap();

    vengine::SceneObject *root1 = scene.addSceneObject("root1", vengine::Transform({0, -8, 0}, {100, 100, 100}));
    root1->add<vengine::ComponentMesh>().setMesh(planeMesh);
    root1->add<vengine::ComponentMaterial>().setMaterial(instanceMaterials.get("defaultMaterial"));

    vengine::SceneObject *root2 =
        scene.addSceneObject("root2", vengine::Transform({1, 0, 0}, {1.1, 1.1, 1.1}, {0.F, glm::radians(15.F), 0.F}));
    root2->add<vengine::ComponentMesh>().setMesh(cubeMesh);
    root2->add<vengine::ComponentMaterial>().setMaterial(instanceMaterials.get("defaultMaterial"));

    vengine::SceneObject *l1_1 = scene.addSceneObject("l1_1", root2, vengine::Transform({0, 3, 0}, {0.5, 0.5, 0.5}));
    l1_1->add<vengine::ComponentMesh>().setMesh(cubeMesh);
    l1_1->add<vengine::ComponentMaterial>().setMaterial(instanceMaterials.get("defaultMaterial"));

    vengine::SceneObject *l1_2 = scene.addSceneObject("l1_2", root2, vengine::Transform({0, -4, 0}, {1, 1, 1}, {glm::radians(20.F), 0.F, 0.F}));
    l1_2->add<vengine::ComponentMesh>().setMesh(cubeMesh);
    l1_2->add<vengine::ComponentMaterial>().setMaterial(instanceMaterials.get("defaultMaterial"));

    vengine::SceneObject *l2_1 =
        scene.addSceneObject("l2_1", l1_1, vengine::Transform({3, 0, 4}, {1, 1, 1}, {glm::radians(45.F), glm::radians(90.F), 0}));
    l2_1->add<vengine::ComponentMesh>().setMesh(cubeMesh);
    l2_1->add<vengine::ComponentMaterial>().setMaterial(instanceMaterials.get("defaultMaterial"));

    vengine::SceneObject *l2_2 = scene.addSceneObject("l2_2", l1_2, vengine::Transform({-3, 0, -4}));
    l2_2->add<vengine::ComponentMesh>().setMesh(cubeMesh);
    l2_2->add<vengine::ComponentMaterial>().setMaterial(instanceMaterials.get("defaultMaterial"));

    auto *directionalLight = engine.scene().createLight(AssetInfo("Directional light", "Directional light"), LightType::DIRECTIONAL_LIGHT);
    static_cast<DirectionalLight *>(directionalLight)->color() = glm::vec4(1);
    auto *dirLight = engine.scene().addSceneObject("Directional light", l2_1, Transform());
    dirLight->add<ComponentLight>().setLight(directionalLight);

    engine.scene().environmentType() = EnvironmentType::HDRI;
    engine.scene().backgroundColor() = glm::vec3(0, 0, 0);
    scene.environmentIntensity() = 0.5F;
    scene.update();

    engine.renderer().rendererPathTracing().renderInfo().filename = "Hierarchy_test";
    engine.renderer().rendererPathTracing().renderInfo().width = 256;
    engine.renderer().rendererPathTracing().renderInfo().height = 256;
    engine.renderer().rendererPathTracing().renderInfo().samples = 2048;
    engine.renderer().rendererPathTracing().renderInfo().batchSize = 64;
    engine.renderer().rendererPathTracing().renderInfo().fileType = vengine::FileType::HDR;
    engine.renderer().rendererPathTracing().renderInfo().denoise = false;
    engine.renderer().rendererPathTracing().renderInfo().writeAllFiles = false;
    engine.renderer().rendererPathTracing().render();

    float diff = CompareImages("Hierarchy_test.hdr", "assets/unittests/Hierarchy_ref.hdr", 3);
    EXPECT_LE(diff, 0.000001);
}

TEST_F(RenderPTTest, DepthOfField)
{
    Engine &engine = *mEngine;
    Scene &scene = engine.scene();

    auto camera = std::make_shared<vengine::PerspectiveCamera>();
    camera->transform().position() = glm::vec3(0, 1, 8);
    camera->transform().setRotation(glm::quat(glm::vec3(glm::radians(0.F), 0, 0)));
    camera->focalDistance() = 8.0F;
    camera->lensRadius() = 0.4F;
    camera->fov() = 60.0f;
    scene.camera() = camera;

    auto &instanceModels = vengine::AssetManager::getInstance().modelsMap();
    auto sphereModel = instanceModels.get("assets/models/uvsphere.obj");
    Mesh *sphereMesh = sphereModel->mesh("defaultobject");
    auto planeModel = instanceModels.get("assets/models/plane.obj");
    Mesh *planeMesh = planeModel->mesh("Plane");
    auto cubeModel = instanceModels.get("assets/models/cube.obj");
    Mesh *cubeMesh = cubeModel->mesh("Cube");

    auto materialPBR = engine.materials().createMaterial<vengine::MaterialPBRStandard>(vengine::AssetInfo("pbrmaterial1"));
    auto materialLambert = engine.materials().createMaterial<vengine::MaterialLambert>(vengine::AssetInfo("lambertmaterial1"));

    {
        vengine::SceneObject *so = scene.addSceneObject("sphere1", vengine::Transform({0, 1, 0}, {1, 1, 1}));
        so->add<vengine::ComponentMesh>().setMesh(sphereMesh);
        so->add<vengine::ComponentMaterial>().setMaterial(materialPBR);
    }
    {
        vengine::SceneObject *so = scene.addSceneObject("sphere2", vengine::Transform({-2, 1, -2}, {1, 1, 1}));
        so->add<vengine::ComponentMesh>().setMesh(sphereMesh);
        so->add<vengine::ComponentMaterial>().setMaterial(materialPBR);
    }
    {
        vengine::SceneObject *so = scene.addSceneObject("sphere3", vengine::Transform({2, 1, 2}, {1, 1, 1}));
        so->add<vengine::ComponentMesh>().setMesh(sphereMesh);
        so->add<vengine::ComponentMaterial>().setMaterial(materialPBR);
    }

    {
        vengine::SceneObject *so = scene.addSceneObject("plane", vengine::Transform({0, 0, 0}, {10, 10, 10}));
        so->add<vengine::ComponentMesh>().setMesh(planeMesh);
        so->add<vengine::ComponentMaterial>().setMaterial(materialLambert);
    }

    scene.environmentType() = vengine::EnvironmentType::HDRI;
    scene.environmentIntensity() = 1.0F;
    scene.update();

    engine.renderer().rendererPathTracing().renderInfo().filename = "DepthOfField_test";
    engine.renderer().rendererPathTracing().renderInfo().width = 256;
    engine.renderer().rendererPathTracing().renderInfo().height = 256;
    engine.renderer().rendererPathTracing().renderInfo().samples = 2048;
    engine.renderer().rendererPathTracing().renderInfo().batchSize = 4;
    engine.renderer().rendererPathTracing().renderInfo().fileType = vengine::FileType::HDR;
    engine.renderer().rendererPathTracing().renderInfo().denoise = false;
    engine.renderer().rendererPathTracing().renderInfo().writeAllFiles = false;
    engine.renderer().rendererPathTracing().render();

    float diff = CompareImages("DepthOfField_test.hdr", "assets/unittests/DepthOfField_ref.hdr", 3);
    EXPECT_LE(diff, 0.0001);
}

TEST_F(RenderPTTest, SharedComponents)
{
    Engine &engine = *mEngine;
    Scene &scene = engine.scene();

    auto camera = std::make_shared<vengine::PerspectiveCamera>();
    camera->fov() = 60.0f;
    camera->transform().position() = glm::vec3(0, 20, 50);
    camera->transform().setRotation(glm::quat(glm::vec3(glm::radians(-15.F), glm::radians(90.F), 0)));
    scene.camera() = camera;

    auto &instanceModels = vengine::AssetManager::getInstance().modelsMap();
    auto &instanceMaterials = vengine::AssetManager::getInstance().materialsMap();

    auto matDef = instanceMaterials.get("defaultMaterial");
    auto cube = instanceModels.get("assets/models/cube.obj");

    vengine::SceneObject *root = scene.addSceneObject("root", nullptr, vengine::Transform());

    vengine::ComponentManager &instance = vengine::ComponentManager::getInstance();
    vengine::ComponentMesh *meshComponent = instance.create<vengine::ComponentMesh, vengine::ComponentOwnerShared>();
    vengine::ComponentMaterial *materialComponent = instance.create<vengine::ComponentMaterial, vengine::ComponentOwnerShared>();

    int32_t size = 150;
    for (int32_t i = -size; i < size; i += 3) {
        for (int32_t j = -size; j < size; j += 3) {
            vengine::SceneObject *so = scene.addSceneObject(
                "cube" + std::to_string(i) + std::to_string(j), nullptr, vengine::Transform({i, 0, j}, {1, 1, 1}));
            so->add_shared<vengine::ComponentMesh>(meshComponent);
            so->add_shared<vengine::ComponentMaterial>(materialComponent);
        }
    }
    meshComponent->setMesh(cube->mesh("Cube"));
    materialComponent->setMaterial(matDef);

    {
        vengine::SceneObject *so = scene.addSceneObject(
            "directionalLight", nullptr, vengine::Transform({0, 2, 0}, {1, 1, 1}, {glm::radians(45.F), glm::radians(90.F), 0}));

        so->add<vengine::ComponentLight>().setLight(vengine::AssetManager::getInstance().lightsMap().get("defaultDirectionalLight"));
    }

    scene.environmentType() = vengine::EnvironmentType::SOLID_COLOR;
    scene.environmentIntensity() = 0.0F;
    scene.update();

    engine.renderer().rendererPathTracing().renderInfo().filename = "SharedComponents_test";
    engine.renderer().rendererPathTracing().renderInfo().width = 256;
    engine.renderer().rendererPathTracing().renderInfo().height = 256;
    engine.renderer().rendererPathTracing().renderInfo().samples = 2048;
    engine.renderer().rendererPathTracing().renderInfo().batchSize = 4;
    engine.renderer().rendererPathTracing().renderInfo().fileType = vengine::FileType::HDR;
    engine.renderer().rendererPathTracing().renderInfo().denoise = false;
    engine.renderer().rendererPathTracing().renderInfo().writeAllFiles = false;
    engine.renderer().rendererPathTracing().render();

    instance.remove(meshComponent);
    instance.remove(materialComponent);

    float diff = CompareImages("SharedComponents_test.hdr", "assets/unittests/SharedComponents_ref.hdr", 3);
    EXPECT_LE(diff, 0.0001);
}