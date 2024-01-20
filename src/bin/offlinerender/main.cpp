#include <iostream>

#include "vengine/vulkan/VulkanEngine.hpp"

#include "PtSceneBallOnPlane.hpp"
#include "PtSceneDragonsOnPlane.hpp"
#include "PtSceneBreakfastRoom.hpp"

using namespace vengine;

int main(int argc, char **argv)
{
    VulkanEngine engine("offlinerender");
    engine.initResources();

    PtSceneBreakfastRoom ptscene(engine);
    ptscene.create();
    ptscene.render();

    engine.releaseResources();

    return 1;
}