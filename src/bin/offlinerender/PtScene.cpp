#include "PtScene.hpp"

PtScene::PtScene(vengine::Engine &engine)
    : m_engine(engine)
    , m_scene(engine.scene())
{
}

vengine::Scene &PtScene::scene()
{
    return m_scene;
}

vengine::Engine &PtScene::engine()
{
    return m_engine;
}
