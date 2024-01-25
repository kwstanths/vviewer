#include "VulkanEngine.hpp"

#include "vulkan/common/IncludeVulkan.hpp"
#include "vulkan/common/VulkanLimits.hpp"

namespace vengine
{

VulkanEngine::VulkanEngine(const std::string &applicationName)
    : Engine()
    , m_context(applicationName)
    , m_textures(m_context)
    , m_materials(m_context)
    , m_swapchain(m_context)
    , m_scene(m_context)
    , m_renderer(m_context, m_swapchain, m_textures, m_materials, m_scene)
{
    m_threadMain = std::thread(&VulkanEngine::mainLoop, this);
}

VulkanEngine::~VulkanEngine()
{
    if (m_status != STATUS::EXITED) {
        exit();
    }
}

void VulkanEngine::initResources()
{
    debug_tools::ConsoleInfo("Initializing resources [OFFLINE MODE]...");

    m_context.init();

    m_scene.initResources();
    m_materials.initResources();
    m_textures.initResources();
    m_renderer.initResources();

    initDefaultData();

    m_scene.initSwapchainResources(1);
    m_materials.initSwapchainResources(1);
    m_textures.initSwapchainResources(1);
}

void VulkanEngine::initResources(VkSurfaceKHR &surface)
{
    debug_tools::ConsoleInfo("Initializing resources...");

    m_context.init(surface);

    m_scene.initResources();
    m_materials.initResources();
    m_textures.initResources();
    m_renderer.initResources();

    initDefaultData();
}

void VulkanEngine::initSwapChainResources(uint32_t windowWidth, uint32_t windowHeight)
{
    assert(!m_context.offlineMode());

    m_swapchain.initResources(windowWidth, windowHeight);

    m_scene.initSwapchainResources(m_swapchain.imageCount());
    m_materials.initSwapchainResources(m_swapchain.imageCount());
    m_textures.initSwapchainResources(m_swapchain.imageCount());
    m_renderer.initSwapChainResources();
}

void VulkanEngine::releaseResources()
{
    if (m_context.offlineMode()) {
        m_textures.releaseSwapchainResources();
        m_materials.releaseSwapchainResources();
        m_scene.releaseSwapchainResources();
    }

    m_renderer.releaseResources();
    m_textures.releaseResources();
    m_materials.releaseResources();
    m_scene.releaseResources();

    m_context.destroy();
}

void VulkanEngine::releaseSwapChainResources()
{
    assert(!m_context.offlineMode());

    m_scene.releaseSwapchainResources();
    m_materials.releaseSwapchainResources();
    m_textures.releaseSwapchainResources();
    m_renderer.releaseSwapChainResources();

    m_swapchain.releaseResources();
}

float VulkanEngine::delta()
{
    if (m_status == STATUS::RUNNING)
        return m_deltaTime;

    return 0.016f;
}

void VulkanEngine::start()
{
    m_threadMainPaused = false;
}

void VulkanEngine::stop()
{
    m_threadMainPaused = true;
}

void VulkanEngine::exit()
{
    m_threadMainExit = true;
    m_threadMain.join();
}

void VulkanEngine::waitIdle()
{
    /* Wait for the main loop to stop or exit */
    for (;;) {
        if (m_status == STATUS::PAUSED || m_status == STATUS::EXITED)
            break;
    }

    /* Wait for the renderer to stop working */
    m_renderer.waitIdle();
}

Model3D *VulkanEngine::importModel(const AssetInfo &info, bool importMaterials)
{
    try {
        auto &modelsMap = AssetManager::getInstance().modelsMap();

        if (modelsMap.isPresent(info.name))
            return modelsMap.get(info.name);

        Tree<ImportedModelNode> importedNode;
        std::vector<Material *> materials = {};
        if (importMaterials) {
            std::vector<ImportedMaterial> importedMaterials;
            importedNode = assimpLoadModel(info, importedMaterials);

            materials = m_materials.createImportedMaterials(importedMaterials, m_textures);
        } else {
            importedNode = assimpLoadModel(info);
        }

        auto vkmodel = new VulkanModel3D(
            info,
            importedNode,
            materials,
            {m_context.physicalDevice(), m_context.device(), m_context.graphicsCommandPool(), m_context.graphicsQueue()},
            true);

        return modelsMap.add(vkmodel);
    } catch (std::runtime_error &e) {
        debug_tools::ConsoleCritical("Failed to import a model: " + std::string(e.what()));
        return nullptr;
    }

    return nullptr;
}

EnvironmentMap *VulkanEngine::importEnvironmentMap(const AssetInfo &info, bool keepTexture)
{
    try {
        /* Check if an environment map for that imagePath already exists */
        auto &envMaps = AssetManager::getInstance().environmentsMapMap();
        if (envMaps.isPresent(info.name)) {
            return envMaps.get(info.name);
        }

        /* Read HDR image */
        VulkanTexture *hdrImage = static_cast<VulkanTexture *>(m_textures.createTextureHDR(info));

        VkResult res;
        /* Transform input texture into a cubemap */
        VulkanCubemap *cubemap, *irradiance, *prefiltered;
        res = m_renderer.getRendererSkybox().createCubemap(hdrImage, cubemap);
        assert(res == VK_SUCCESS);
        /* Compute irradiance map */
        res = m_renderer.getRendererSkybox().createIrradianceMap(cubemap, irradiance);
        assert(res == VK_SUCCESS);
        /* Compute prefiltered map */
        res = m_renderer.getRendererSkybox().createPrefilteredCubemap(cubemap, prefiltered);
        assert(res == VK_SUCCESS);

        if (!keepTexture) {
            auto &textures = AssetManager::getInstance().texturesMap();
            textures.remove(info.filepath);
            hdrImage->destroy(m_context.device());
        }

        auto envMap = new EnvironmentMap(info, cubemap, irradiance, prefiltered);
        return envMaps.add(envMap);
    } catch (std::runtime_error &e) {
        debug_tools::ConsoleCritical("Failed to import an environment map: " + std::string(e.what()));
        return nullptr;
    }

    return nullptr;
}

void VulkanEngine::deleteImportedAssets()
{
    auto &modelsMap = AssetManager::getInstance().modelsMap();
    for (auto itr = modelsMap.begin(); itr != modelsMap.end();) {
        if (!itr->second->internal()) {
            auto model = static_cast<VulkanModel3D *>(itr->second);
            model->destroy(m_context.device());
            itr = modelsMap.remove(itr->second);
            delete model;
        } else {
            ++itr;
        }
    }

    auto &materialsMap = AssetManager::getInstance().materialsMap();
    for (auto itr = materialsMap.begin(); itr != materialsMap.end();) {
        if (!itr->second->internal()) {
            auto material = static_cast<Material *>(itr->second);
            itr = materialsMap.remove(itr->second);
            delete material;
        } else {
            ++itr;
        }
    }

    auto &texturesMap = AssetManager::getInstance().texturesMap();
    for (auto itr = texturesMap.begin(); itr != texturesMap.end();) {
        if (!itr->second->internal()) {
            auto texture = static_cast<VulkanTexture *>(itr->second);
            itr = texturesMap.remove(itr->second);
            texture->destroy(m_context.device());
            delete texture;
        } else {
            ++itr;
        }
    }

    auto &lightsMap = AssetManager::getInstance().lightsMap();
    for (auto itr = lightsMap.begin(); itr != lightsMap.end();) {
        if (!itr->second->internal()) {
            auto light = static_cast<Light *>(itr->second);
            itr = lightsMap.remove(itr->second);
            delete light;
        } else {
            ++itr;
        }
    }
}

void VulkanEngine::mainLoop()
{
    for (;;) {
        std::this_thread::sleep_for(std::chrono::milliseconds(16));

        if (m_threadMainExit) {
            break;
        }

        if (m_threadMainPaused) {
            m_status = STATUS::PAUSED;
            continue;
        }

        m_status = STATUS::RUNNING;

        /* update scene */
        m_scene.updateSceneGraph();

        /* parse scene */
        SceneGraph sceneGraphArray = m_scene.getSceneObjectsArray();

        /* Calculate delta time */
        auto currentTime = std::chrono::steady_clock::now();
        m_deltaTime = (std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - m_frameTimePrev).count()) / 1000.0f;
        m_frameTimePrev = currentTime;

        /* Render frame */
        VULKAN_WARNING(m_renderer.renderFrame(sceneGraphArray));
    }

    m_status = STATUS::EXITED;
}

void VulkanEngine::initDefaultData()
{
    /* A default material */
    {
        auto defaultMaterial = static_cast<VulkanMaterialPBRStandard *>(
            m_materials.createMaterial(AssetInfo("defaultMaterial", AssetSource::INTERNAL), MaterialType::MATERIAL_PBR_STANDARD));
        defaultMaterial->albedo() = glm::vec4(0.8, 0.8, 0.8, 1);
        defaultMaterial->metallic() = 0.5;
        defaultMaterial->roughness() = 0.5;
        defaultMaterial->ao() = 1.0f;
        defaultMaterial->emissive() = glm::vec4(0.0, 0.0, 0.0, 1.0);

        auto defaultEmissive = static_cast<VulkanMaterialPBRStandard *>(
            m_materials.createMaterial(AssetInfo("defaultEmissive", AssetSource::INTERNAL), MaterialType::MATERIAL_PBR_STANDARD));
        defaultEmissive->albedo() = glm::vec4(1, 1, 1, 1);
        defaultEmissive->emissive() = glm::vec4(1, 1, 1, 1.0);
    }

    /* Create some default lights */
    {
        auto &lights = AssetManager::getInstance().lightsMap();

        auto defaultPointLight = scene().createLight(AssetInfo("defaultPointLight", AssetSource::INTERNAL), LightType::POINT_LIGHT);
        static_cast<PointLight *>(defaultPointLight)->color() = glm::vec4(1, 1, 1, 1);
        lights.add(defaultPointLight);

        auto defaultDirectionalLight =
            scene().createLight(AssetInfo("defaultDirectionalLight", AssetSource::INTERNAL), LightType::DIRECTIONAL_LIGHT);
        static_cast<PointLight *>(defaultDirectionalLight)->color() = glm::vec4(1, 0.9, 0.8, 1);
        lights.add(defaultDirectionalLight);
    }

    {
        /* Some models */
        auto uvsphereMeshModel = importModel(AssetInfo("assets/models/uvsphere.obj", AssetSource::INTERNAL), false);
        auto planeMeshModel = importModel(AssetInfo("assets/models/plane.obj", AssetSource::INTERNAL), false);
        auto cubeMeshModel = importModel(AssetInfo("assets/models/cube.obj", AssetSource::INTERNAL), false);
    }

    /* A skybox material */
    {
        auto envMap = importEnvironmentMap(AssetInfo("assets/HDR/harbor.hdr", AssetSource::INTERNAL));

        auto &materialsSkybox = AssetManager::getInstance().materialsSkyboxMap();
        auto skybox = materialsSkybox.add(new VulkanMaterialSkybox(AssetInfo("skybox", AssetSource::INTERNAL),
                                                                   envMap,
                                                                   m_context.device(),
                                                                   m_renderer.getRendererSkybox().descriptorSetLayout()));

        m_scene.skyboxMaterial() = skybox;
    }
}

}  // namespace vengine