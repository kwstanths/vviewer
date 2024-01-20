#ifndef __PtSceneBallOnPlane_hpp__
#define __PtSceneBallOnPlane_hpp__

#include "PtScene.hpp"

class PtSceneBallOnPlane : public PtScene
{
public:
    PtSceneBallOnPlane(vengine::Engine &engine);

    bool create() override;
    bool render() override;

private:
};

#endif /* __PtSceneBallOnPlane_hpp__ */
