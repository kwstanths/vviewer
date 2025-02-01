#include <iostream>

#include "vengine/vulkan/VulkanEngine.hpp"

#include "PtSceneBallOnPlane.hpp"
#include "PtSceneDragonsOnPlane.hpp"
#include "PtSceneBreakfastRoom.hpp"
#include "PtSceneFurnace.hpp"
#include "PtSceneSharedComponents.hpp"

using namespace vengine;

int main(int argc, char **argv)
{
    VulkanEngine engine("offlinerender");
    engine.initResources();

    PtSceneDragonsOnPlane ptscene(engine);
    ptscene.create();
    ptscene.render();

    engine.releaseResources();

    return 1;
}