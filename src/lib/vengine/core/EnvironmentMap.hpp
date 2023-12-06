#ifndef __EnvironmentMap_hpp__
#define __EnvironmentMap_hpp__

#include <memory>

#include "Asset.hpp"
#include "Cubemap.hpp"

namespace vengine
{

class EnvironmentMap : public Asset
{
public:
    EnvironmentMap(const AssetInfo &info, Cubemap *skybox, Cubemap *irradiance, Cubemap *prefilteredMap)
        : Asset(info)
        , m_skyboxMap(skybox)
        , m_irradianceMap(irradiance)
        , m_prefilteredMap(prefilteredMap){};

    Cubemap *skyboxMap() const;
    Cubemap *irradianceMap() const;
    Cubemap *prefilteredMap() const;

private:
    Cubemap *m_skyboxMap;
    Cubemap *m_irradianceMap;
    Cubemap *m_prefilteredMap;
};

}  // namespace vengine

#endif