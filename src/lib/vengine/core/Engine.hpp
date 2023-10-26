#ifndef __Engine_hpp__
#define __Engine_hpp__

#include "Scene.hpp"
#include "Renderer.hpp"
#include "Model3D.hpp"
#include "EnvironmentMap.hpp"
#include "Textures.hpp"
#include "Materials.hpp"

namespace vengine
{

class Engine
{
public:
    enum class STATUS {
        NOT_STARTED = 0,
        RUNNING = 1,
        PAUSED = 2,
        EXITED = 3,
    };

    Engine(){};
    virtual ~Engine(){};

    STATUS status() { return m_status; }

    virtual Scene &scene() = 0;
    virtual Renderer &renderer() = 0;
    virtual Textures &textures() = 0;
    virtual Materials &materials() = 0;

    virtual float delta() = 0;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void exit() = 0;
    virtual void waitIdle() = 0;

    virtual std::shared_ptr<Model3D> importModel(std::string filename, bool importMaterials = true) = 0;
    virtual std::shared_ptr<EnvironmentMap> importEnvironmentMap(std::string imagePath, bool keepTexture = false) = 0;

protected:
    STATUS m_status = STATUS::NOT_STARTED;
};

}  // namespace vengine

#endif