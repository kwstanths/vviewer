#ifndef __PtScene_hpp__
#define __PtScene_hpp__

#include "vengine/vulkan/VulkanEngine.hpp"

class PtScene
{
public:
    PtScene(vengine::Engine &engine);

    virtual bool create() = 0;
    virtual bool render() = 0;

protected:
    vengine::Scene &scene();
    vengine::Engine &engine();

private:
    vengine::Engine &m_engine;
    vengine::Scene &m_scene;
};

#endif /* __PtScene_hpp__ */
