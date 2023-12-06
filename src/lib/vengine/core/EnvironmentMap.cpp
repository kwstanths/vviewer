#include "EnvironmentMap.hpp"

namespace vengine
{

Cubemap *EnvironmentMap::skyboxMap() const
{
    return m_skyboxMap;
}

Cubemap *EnvironmentMap::irradianceMap() const
{
    return m_irradianceMap;
}

Cubemap *EnvironmentMap::prefilteredMap() const
{
    return m_prefilteredMap;
}

}  // namespace vengine