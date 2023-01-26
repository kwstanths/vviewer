#ifndef __Lights_hpp__
#define __Lights_hpp__

#include <string>

#include "glm/glm.hpp"

#include <math/Transform.hpp>
#include <utils/ECS.hpp>

struct LightMaterial {
    std::string name;
    glm::vec3 color;
    float intensity;

    LightMaterial(std::string n, const glm::vec3& c, float i) : name(n), color(c), intensity(i) {};
};

struct Light {
    LightMaterial * lightMaterial;

    Light(LightMaterial * lm) : lightMaterial(lm) {};
};

struct DirectionalLight : public Light 
{
    Transform transform;
    
    DirectionalLight(Transform t, LightMaterial * lm) : Light(lm), transform(t) {};
};

struct PointLight : public Light, public Component
{
    PointLight(LightMaterial* lm): Light(lm), Component(ComponentType::POINT_LIGHT) {};
};

static float squareFalloff(glm::vec3 a, glm::vec3 b) 
{
    float distance = glm::length(a - b);
    return 1.0F / (distance * distance);
}

#endif