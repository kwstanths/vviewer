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
    , m_scene(m_context, VULKAN_LIMITS_MAX_OBJECTS)
    , m_renderer(m_context, m_swapchain, m_textures, m_materials, m_scene)
{
    m_threadMain = std::thread(&VulkanEngine::mainLoop, this);
}

VulkanEngine::~VulkanEngine()
{
}

bool VulkanEngine::setSurface(VkSurfaceKHR &surface)
{
    m_context.init(surface);

    return true;
}

void VulkanEngine::initResources()
{
    m_scene.initResources();
    m_materials.initResources();
    m_textures.initResources();
    m_renderer.initResources();

    /* Create a default material */
    auto defaultMaterial = std::static_pointer_cast<VulkanMaterialPBRStandard>(
        m_materials.createMaterial("defaultMaterial", MaterialType::MATERIAL_PBR_STANDARD, false));
    defaultMaterial->albedo() = glm::vec4(0.8, 0.8, 0.8, 1);
    defaultMaterial->metallic() = 0.5;
    defaultMaterial->roughness() = 0.5;
    defaultMaterial->ao() = 1.0f;
    defaultMaterial->emissive() = glm::vec4(0.0, 0.0, 0.0, 1.0);

    /* Import some models */
    auto uvsphereMeshModel = importModel("assets/models/uvsphere.obj", false);
    auto planeMeshModel = importModel("assets/models/plane.obj", false);
    auto cubeMeshModel = importModel("assets/models/cube.obj", false);

    /* Create a skybox material */
    {
        auto envMap = importEnvironmentMap("assets/HDR/harbor.hdr");

        auto &materialsSkybox = AssetManager::getInstance().materialsSkyboxMap();
        auto skybox = materialsSkybox.add(std::make_shared<VulkanMaterialSkybox>(
            "skybox", envMap, m_context.device(), m_renderer.getRendererSkybox().descriptorSetLayout()));

        m_scene.setSkybox(skybox);
    }
}

void VulkanEngine::initSwapChainResources(uint32_t windowWidth, uint32_t windowHeight)
{
    m_swapchain.initResources(windowWidth, windowHeight);

    m_scene.initSwapchainResources(m_swapchain.imageCount());
    m_materials.initSwapchainResources(m_swapchain.imageCount());
    m_textures.initSwapchainResources(m_swapchain.imageCount());
    m_renderer.initSwapChainResources();
}

void VulkanEngine::releaseResources()
{
    m_scene.releaseResources();
    m_materials.releaseResources();
    m_textures.releaseResources();
    m_renderer.releaseResources();
}

void VulkanEngine::releaseSwapChainResources()
{
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

std::shared_ptr<Model3D> VulkanEngine::importModel(std::string filename, bool importMaterials)
{
    try {
        auto &modelsMap = AssetManager::getInstance().modelsMap();

        if (modelsMap.isPresent(filename))
            return modelsMap.get(filename);

        Tree<ImportedModelNode> importedNode;
        std::vector<std::shared_ptr<Material>> materials = {};
        if (importMaterials) {
            std::vector<ImportedMaterial> importedMaterials;
            importedNode = assimpLoadModel(filename, importedMaterials);

            materials = m_materials.createImportedMaterials(importedMaterials, m_textures);
        } else {
            importedNode = assimpLoadModel(filename);
        }

        /* Check if ray tracing is enabled, and add extra buffer usage flags */
        VkBufferUsageFlags extraUsageFlags = {};
        if (m_renderer.rendererRayTracing().isRTEnabled()) {
            extraUsageFlags = static_cast<VulkanRendererRayTracing &>(m_renderer.rendererRayTracing()).getBufferUsageFlags();
        }

        auto vkmodel = std::make_shared<VulkanModel3D>(filename,
                                                       importedNode,
                                                       materials,
                                                       m_context.physicalDevice(),
                                                       m_context.device(),
                                                       m_context.graphicsQueue(),
                                                       m_context.graphicsCommandPool(),
                                                       extraUsageFlags);

        return modelsMap.add(vkmodel);
    } catch (std::runtime_error &e) {
        debug_tools::ConsoleCritical("Failed to import a model: " + std::string(e.what()));
        return nullptr;
    }

    return nullptr;
}

std::shared_ptr<EnvironmentMap> VulkanEngine::importEnvironmentMap(std::string imagePath, bool keepTexture)
{
    try {
        /* Check if an environment map for that imagePath already exists */
        auto &envMaps = AssetManager::getInstance().environmentsMapMap();
        if (envMaps.isPresent(imagePath)) {
            return envMaps.get(imagePath);
        }

        /* Read HDR image */
        std::shared_ptr<VulkanTexture> hdrImage = std::static_pointer_cast<VulkanTexture>(m_textures.createTextureHDR(imagePath));

        VkResult res;
        /* Transform input texture into a cubemap */
        std::shared_ptr<VulkanCubemap> cubemap, irradiance, prefiltered;
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
            textures.remove(imagePath);
            hdrImage->destroy(m_context.device());
        }

        auto envMap = std::make_shared<EnvironmentMap>(imagePath, cubemap, irradiance, prefiltered);
        return envMaps.add(envMap);
    } catch (std::runtime_error &e) {
        debug_tools::ConsoleCritical("Failed to import an environment map: " + std::string(e.what()));
        return nullptr;
    }

    return nullptr;
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

}  // namespace vengine