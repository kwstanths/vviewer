#include "EnvironmentMap.hpp"

Cubemap* EnvironmentMap::getSkyboxMap() const
{
    return m_skyboxMap;
}

Cubemap* EnvironmentMap::getIrradianceMap() const
{
    return m_irradianceMap;
}
