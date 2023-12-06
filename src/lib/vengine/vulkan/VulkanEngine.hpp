#ifndef __VulkanEngine_hpp__
#define __VulkanEngine_hpp__

#include <vengine/core/Engine.hpp>
#include "renderers/VulkanRenderer.hpp"

#include "VulkanContext.hpp"
#include "VulkanScene.hpp"
#include "VulkanSwapchain.hpp"

namespace vengine
{

class VulkanEngine : public Engine
{
public:
    VulkanEngine(const std::string &applicationName);
    ~VulkanEngine();

    void initResources();
    void initResources(VkSurfaceKHR &surface);
    void initSwapChainResources(uint32_t windowWidth, uint32_t windowHeight);
    void releaseResources();
    void releaseSwapChainResources();

    inline VulkanContext &context() { return m_context; }
    inline VulkanSwapchain &swapchain() { return m_swapchain; }

    Scene &scene() override { return m_scene; }
    Renderer &renderer() override { return m_renderer; }
    Textures &textures() override { return m_textures; }
    Materials &materials() override { return m_materials; }

    float delta() override;

    void start() override;
    void stop() override;
    void exit() override;
    void waitIdle() override;

    Model3D *importModel(const AssetInfo &info, bool importMaterials = true) override;
    EnvironmentMap *importEnvironmentMap(const AssetInfo &info, bool keepTexture = false) override;

    void deleteImportedAssets() override;

private:
    VulkanContext m_context;
    VulkanTextures m_textures;
    VulkanMaterials m_materials;
    VulkanSwapchain m_swapchain;
    VulkanScene m_scene;
    VulkanRenderer m_renderer;

    /* Engine loop data */
    std::thread m_threadMain;
    bool m_threadMainPaused = true;
    bool m_threadMainExit = false;
    void mainLoop();

    /* Delta time data */
    std::chrono::steady_clock::time_point m_frameTimePrev;
    float m_deltaTime = 0.016F;

    void initDefaultData();
};

}  // namespace vengine

#endif