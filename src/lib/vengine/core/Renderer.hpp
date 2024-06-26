#ifndef __Renderer_hpp__
#define __Renderer_hpp__

#include "Scene.hpp"
#include "SceneObject.hpp"
#include "io/FileTypes.hpp"

namespace vengine
{

class RendererPathTracing
{
public:
    struct RenderInfo {
        uint32_t samples = 256u;
        uint32_t depth = 9u;
        uint32_t batchSize = 4u;

        std::string filename = "test";
        FileType fileType = FileType::HDR;
        float exposure = 0.0F;

        uint32_t width = 1080u;
        uint32_t height = 1080u;

        bool denoise = false;
        bool writeAllFiles = false;
    };

    RenderInfo &renderInfo() { return m_renderInfo; }
    const RenderInfo &renderInfo() const { return m_renderInfo; }

    virtual bool isRayTracingEnabled() const = 0;
    virtual void render() = 0;
    virtual float renderProgress() = 0;

private:
    RenderInfo m_renderInfo;
};

class Renderer
{
public:
    Renderer();

    void setSelectedObject(SceneObject *sceneObject) { m_selectedObject = sceneObject; }
    SceneObject *getSelectedObject() const { return m_selectedObject; }

    virtual RendererPathTracing &rendererPathTracing() = 0;

    bool &showSelectedAABB() { return m_showSelectedAABB; }
    const bool &showSelectedAABB() const { return m_showSelectedAABB; }

protected:
    SceneObject *m_selectedObject = nullptr;

    bool m_showSelectedAABB = true;

private:
};

}  // namespace vengine

#endif