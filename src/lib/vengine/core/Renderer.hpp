#ifndef __Renderer_hpp__
#define __Renderer_hpp__

#include "Scene.hpp"
#include "SceneObject.hpp"
#include "io/FileTypes.hpp"

namespace vengine
{

class RendererRayTracing
{
public:
    struct RenderInfo {
        uint32_t samples = 256u;
        uint32_t depth = 5u;
        uint32_t batchSize = 16u;

        std::string filename = "test";
        FileType fileType = FileType::HDR;

        uint32_t width = 1024u;
        uint32_t height = 1024u;
    };

    RenderInfo &renderInfo() { return m_renderInfo; }
    const RenderInfo &renderInfo() const { return m_renderInfo; }

    virtual bool isRTEnabled() const = 0;
    virtual void render(const Scene &scene) = 0;
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

    virtual RendererRayTracing &rendererRayTracing() = 0;

protected:
    SceneObject *m_selectedObject = nullptr;

private:
};

}  // namespace vengine

#endif