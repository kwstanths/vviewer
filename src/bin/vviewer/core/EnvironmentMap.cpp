#include "EnvironmentMap.hpp"

std::shared_ptr<Cubemap> EnvironmentMap::getSkyboxMap() const
{
    return m_skyboxMap;
}

std::shared_ptr<Cubemap> EnvironmentMap::getIrradianceMap() const
{
    return m_irradianceMap;
}

std::shared_ptr<Cubemap> EnvironmentMap::getPrefilteredMap() const
{
    return m_prefilteredMap;
}
