#ifndef __Lights_hpp__
#define __Lights_hpp__

#include <string>
#include <memory>

#include "glm/glm.hpp"

#include <math/Transform.hpp>

namespace vengine {

enum class LightType {
    POINT_LIGHT = 0,
    DIRECTIONAL_LIGHT = 1,
    MESH_LIGHT = 2,
    ENVIRONMENT_MAP = 3,
};

struct LightMaterial {
    std::string name;
    glm::vec3 color;
    float intensity;

    LightMaterial(std::string n, const glm::vec3& c, float i) : name(n), color(c), intensity(i) {};
};

struct Light {
    std::string name;
    LightType type;
    std::shared_ptr<LightMaterial> lightMaterial;

    Light(std::string n, LightType t, std::shared_ptr<LightMaterial>& lm) : name(n), type(t), lightMaterial(lm) {};
};

static float squareFalloff(glm::vec3 a, glm::vec3 b) 
{
    float distance = glm::length(a - b);
    return 1.0F / (distance * distance);
}

}

#endif