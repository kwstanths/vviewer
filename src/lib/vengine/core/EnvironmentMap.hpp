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
    EnvironmentMap(std::string name, Cubemap *skybox, Cubemap *irradiance, Cubemap *prefilteredMap)
        : Asset(name)
        , m_skyboxMap(skybox)
        , m_irradianceMap(irradiance)
        , m_prefilteredMap(prefilteredMap){};

    Cubemap *getSkyboxMap() const;
    Cubemap *getIrradianceMap() const;
    Cubemap *getPrefilteredMap() const;

private:
    Cubemap *m_skyboxMap;
    Cubemap *m_irradianceMap;
    Cubemap *m_prefilteredMap;
};

}  // namespace vengine

#endif