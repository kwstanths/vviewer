#ifndef __VulkanEngine_hpp__
#define __VulkanEngine_hpp__

#include <core/Engine.hpp>

#include "VulkanContext.hpp"
#include "VulkanScene.hpp"
#include "VulkanSwapchain.hpp"
#include "renderers/VulkanRenderer.hpp"

namespace vengine {

class VulkanEngine : public Engine {
public:
    VulkanEngine();
    ~VulkanEngine();
    
    bool setSurface(VkSurfaceKHR& surface);

    void initResources();
    void initSwapChainResources(uint32_t windowWidth, uint32_t windowHeight);
    void releaseResources();
    void releaseSwapChainResources();

    inline VulkanContext& context() { return m_context; }
    inline VulkanTextures& textures() { return m_textures; }
    inline VulkanMaterials& materials() { return m_materials; }
    inline VulkanSwapchain& swapchain() { return m_swapchain; }

    Scene * scene() override { return m_scene; }
    Renderer * renderer() override { return m_renderer; }

    float delta() override;

    void start() override;
    void stop() override;
    void exit() override;
    void waitIdle() override;

private:
    VulkanContext m_context;
    VulkanTextures m_textures;
    VulkanMaterials m_materials;
    VulkanSwapchain m_swapchain;

    VulkanScene * m_scene = nullptr;
    VulkanRenderer * m_renderer = nullptr;

    /* Engine loop data */
    std::thread m_threadMain;
    bool m_threadMainPaused = true;
    bool m_threadMainExit = false;
    void mainLoop();

    /* Delta time data */
    std::chrono::steady_clock::time_point m_frameTimePrev;
    float m_deltaTime = 0.016F;

};

}

#endif