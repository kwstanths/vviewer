#ifndef __PtSceneBreakfastRoom_hpp__
#define __PtSceneBreakfastRoom_hpp__

#include "PtScene.hpp"

class PtSceneBreakfastRoom : public PtScene
{
public:
    PtSceneBreakfastRoom(vengine::Engine &engine);

    bool create() override;
    bool render() override;

private:
};

#endif /* __PtSceneBreakfastRoom_hpp__ */
