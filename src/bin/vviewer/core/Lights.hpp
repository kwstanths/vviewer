#ifndef __Lights_hpp__
#define __Lights_hpp__

#include "glm/glm.hpp"

#include <math/Transform.hpp>
#include <utils/ECS.hpp>

struct Light {
    glm::vec3 color;

    Light(glm::vec3 c) : color(c) {};
};

struct DirectionalLight : public Light 
{
    Transform transform;
    
    DirectionalLight(Transform t, glm::vec3 c) : Light(c), transform(t) {};
};

struct PointLight : public Light, public Component
{
    PointLight(glm::vec3 c): Light(c), Component(ComponentType::POINT_LIGHT) {};
};

#endif