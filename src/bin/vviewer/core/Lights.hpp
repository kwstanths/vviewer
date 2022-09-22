#ifndef __Lights_hpp__
#define __Lights_hpp__

#include "glm/glm.hpp"

#include <math/Transform.hpp>
#include <utils/ECS.hpp>

struct Light {
    glm::vec3 color;
    float intensity;

    Light(glm::vec3 c, float i) : color(c), intensity(i) {};
};

struct DirectionalLight : public Light 
{
    Transform transform;
    
    DirectionalLight(Transform t, glm::vec3 color, float intensity) : Light(color, intensity), transform(t) {};
};

struct PointLight : public Light, public Component
{
    PointLight(glm::vec3 color, float intensity): Light(color, intensity), Component(ComponentType::POINT_LIGHT) {};
};

#endif