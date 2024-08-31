#ifndef __PtSceneBallFurnace_hpp__
#define __PtSceneBallFurnace_hpp__

#include "PtScene.hpp"

class PtSceneFurnace : public PtScene
{
public:
    PtSceneFurnace(vengine::Engine &engine);

    bool create() override;
    bool render() override;

private:
};

#endif
