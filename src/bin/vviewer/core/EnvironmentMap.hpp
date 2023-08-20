#ifndef __EnvironmentMap_hpp__
#define __EnvironmentMap_hpp__

#include <memory>

#include "Cubemap.hpp"

namespace vengine {

class EnvironmentMap {
public:
    EnvironmentMap(std::string name, std::shared_ptr<Cubemap> skybox, std::shared_ptr<Cubemap> irradiance, std::shared_ptr<Cubemap> prefilteredMap)
        : m_name(name), m_skyboxMap(skybox), m_irradianceMap(irradiance), m_prefilteredMap(prefilteredMap) {};

    std::string m_name;

    std::shared_ptr<Cubemap> getSkyboxMap() const;
    std::shared_ptr<Cubemap> getIrradianceMap() const;
    std::shared_ptr<Cubemap> getPrefilteredMap() const;

private:
    std::shared_ptr<Cubemap> m_skyboxMap;
    std::shared_ptr<Cubemap> m_irradianceMap;
    std::shared_ptr<Cubemap> m_prefilteredMap;
};

}

#endif