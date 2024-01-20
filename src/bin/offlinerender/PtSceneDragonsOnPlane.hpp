#ifndef __PtSceneDragonsOnPlane_hpp__
#define __PtSceneDragonsOnPlane_hpp__

#include "PtScene.hpp"

class PtSceneDragonsOnPlane : public PtScene
{
public:
    PtSceneDragonsOnPlane(vengine::Engine &engine);

    bool create() override;
    bool render() override;

private:
};

#endif /* __PtSceneDragonsOnPlane_hpp__ */
