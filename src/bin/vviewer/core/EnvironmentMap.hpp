#ifndef __EnvironmentMap_hpp__
#define __EnvironmentMap_hpp__

#include "Cubemap.hpp"

class EnvironmentMap {
public:
    EnvironmentMap(std::string name, Cubemap* skybox, Cubemap* irradiance, Cubemap* prefilteredMap)
        : m_name(name), m_skyboxMap(skybox), m_irradianceMap(irradiance), m_prefilteredMap(prefilteredMap) {};

    std::string m_name;

    Cubemap* getSkyboxMap() const;
    Cubemap* getIrradianceMap() const;
    Cubemap* getPrefilteredMap() const;

private:
    Cubemap* m_skyboxMap;
    Cubemap* m_irradianceMap;
    Cubemap* m_prefilteredMap;
};

#endif