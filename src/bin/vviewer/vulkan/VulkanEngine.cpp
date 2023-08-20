#include "VulkanEngine.hpp"

#include "vulkan/common/IncludeVulkan.hpp"
#include "vulkan/common/VulkanLimits.hpp"

namespace vengine
{

VulkanEngine::VulkanEngine()
    : Engine()
    , m_textures(m_context)
    , m_materials(m_context)
    , m_swapchain(m_context)
{
    m_scene = new VulkanScene(m_context, VULKAN_LIMITS_MAX_OBJECTS);
    m_renderer = new VulkanRenderer(m_context, m_swapchain, m_textures, m_materials, m_scene);

    m_threadMain = std::thread(&VulkanEngine::mainLoop, this);
}

VulkanEngine::~VulkanEngine()
{
    delete m_renderer;
    delete m_scene;
}

bool VulkanEngine::setSurface(VkSurfaceKHR &surface)
{
    m_context.init(surface);

    return true;
}

void VulkanEngine::initResources()
{
    m_scene->initResources();
    m_materials.initResources();
    m_textures.initResources();
    m_renderer->initResources();
}

void VulkanEngine::initSwapChainResources(uint32_t windowWidth, uint32_t windowHeight)
{
    m_swapchain.initResources(windowWidth, windowHeight);

    m_scene->initSwapchainResources(m_swapchain.imageCount());
    m_materials.initSwapchainResources(m_swapchain.imageCount());
    m_textures.initSwapchainResources(m_swapchain.imageCount());
    m_renderer->initSwapChainResources();
}

void VulkanEngine::releaseResources()
{
    m_scene->releaseResources();
    m_materials.releaseResources();
    m_textures.releaseResources();
    m_renderer->releaseResources();
}

void VulkanEngine::releaseSwapChainResources()
{
    m_scene->releaseSwapchainResources();
    m_materials.releaseSwapchainResources();
    m_textures.releaseSwapchainResources();
    m_renderer->releaseSwapChainResources();

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
    m_renderer->waitIdle();
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
        m_scene->updateSceneGraph();

        /* parse scene */
        SceneGraph sceneGraphArray = m_scene->getSceneObjectsArray();

        /* Calculate delta time */
        auto currentTime = std::chrono::steady_clock::now();
        m_deltaTime = (std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - m_frameTimePrev).count()) / 1000.0f;
        m_frameTimePrev = currentTime;

        /* Render frame */
        VULKAN_WARNING(m_renderer->renderFrame(sceneGraphArray));
    }

    m_status = STATUS::EXITED;
}

}  // namespace vengine