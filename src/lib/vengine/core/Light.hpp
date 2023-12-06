#ifndef __Light_hpp__
#define __Light_hpp__

#include <string>
#include <memory>
#include <unordered_map>

#include "glm/glm.hpp"

#include "Asset.hpp"
#include <vengine/math/Transform.hpp>

namespace vengine
{

enum class LightType {
    POINT_LIGHT = 0,
    DIRECTIONAL_LIGHT = 1,
    MESH_LIGHT = 2,
};

static const std::unordered_map<LightType, std::string> lightTypeNames = {
    {LightType::POINT_LIGHT, "Point light"},
    {LightType::DIRECTIONAL_LIGHT, "Directional light"},
};

typedef uint32_t LightIndex;

class Light : public Asset
{
public:
    Light(const AssetInfo &info)
        : Asset(info){};

    virtual ~Light(){};

    virtual LightType type() const = 0;

    virtual LightIndex lightIndex() const = 0;

    /**
     * @brief The light contribution of such a light on position l, on point p
     *
     * @param l
     * @param point
     * @return float
     */
    virtual float power(const glm::vec3 &l, const glm::vec3 &p) const = 0;

private:
};

class PointLight : public Light
{
public:
    PointLight(const AssetInfo &info)
        : Light(info){};

    virtual ~PointLight(){};

    LightType type() const { return LightType::POINT_LIGHT; }

    /* RGB = color, A = intensity */
    virtual glm::vec4 &color() = 0;
    virtual const glm::vec4 &color() const = 0;

    float power(const glm::vec3 &l, const glm::vec3 &p) const
    {
        float distance = glm::distance(l, p);
        return color().a / (distance * distance);
    }

private:
};

class DirectionalLight : public Light
{
public:
    DirectionalLight(const AssetInfo &info)
        : Light(info){};

    virtual ~DirectionalLight(){};

    LightType type() const { return LightType::DIRECTIONAL_LIGHT; }

    /* RGB = color, A = intensity */
    virtual glm::vec4 &color() = 0;
    virtual const glm::vec4 &color() const = 0;

    float power(const glm::vec3 &l, const glm::vec3 &p) const { return color().a; }

private:
};

}  // namespace vengine

#endif