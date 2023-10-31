#include "EnvironmentMap.hpp"

namespace vengine
{

Cubemap *EnvironmentMap::getSkyboxMap() const
{
    return m_skyboxMap;
}

Cubemap *EnvironmentMap::getIrradianceMap() const
{
    return m_irradianceMap;
}

Cubemap *EnvironmentMap::getPrefilteredMap() const
{
    return m_prefilteredMap;
}

}  // namespace vengine