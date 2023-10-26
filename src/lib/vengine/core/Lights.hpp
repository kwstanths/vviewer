#ifndef __Lights_hpp__
#define __Lights_hpp__

#include <string>
#include <memory>

#include "glm/glm.hpp"

#include "Asset.hpp"
#include <vengine/math/Transform.hpp>

namespace vengine
{

enum class LightType {
    POINT_LIGHT = 0,
    DIRECTIONAL_LIGHT = 1,
};

struct LightMaterial : public Asset {
    glm::vec3 color;
    float intensity;

    LightMaterial(std::string name, const glm::vec3 &c, float i)
        : Asset(name)
        , color(c)
        , intensity(i){};
};

struct Light : public Asset {
    LightType type;
    std::shared_ptr<LightMaterial> lightMaterial;

    Light(std::string name, LightType t, std::shared_ptr<LightMaterial> &lm)
        : Asset(name)
        , type(t)
        , lightMaterial(lm){};
};

static float squareFalloff(glm::vec3 a, glm::vec3 b)
{
    float distance = glm::length(a - b);
    return 1.0F / (distance * distance);
}

}  // namespace vengine

#endif