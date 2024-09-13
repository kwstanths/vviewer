#ifndef __PtSceneSharedComponents_hpp__
#define __PtSceneSharedComponents_hpp__

#include "PtScene.hpp"

class PtSceneSharedComponents : public PtScene
{
public:
    PtSceneSharedComponents(vengine::Engine &engine);

    bool create() override;
    bool render() override;

private:
};

#endif
