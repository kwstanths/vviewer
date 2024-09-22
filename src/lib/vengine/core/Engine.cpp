#include "Engine.hpp"

namespace vengine {

Engine::Engine()
{
    const auto processor_count = std::thread::hardware_concurrency();
    m_threadPool.init(processor_count);
}

}