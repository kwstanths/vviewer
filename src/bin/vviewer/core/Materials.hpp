#ifndef __Material_hpp__
#define __Material_hpp__

#include <string>
#include <unordered_map>

#include <glm/glm.hpp>

enum MaterialType {
    MATERIAL_NOT_SET = -1,
    MATERIAL_PBR_STANDARD = 0,
};

static const std::unordered_map<MaterialType, std::string> materialTypeNames = {
    { MaterialType::MATERIAL_PBR_STANDARD, "PBR standard" }
};

class Material {
public:
    Material() {};
    Material(std::string name) : m_name(name) {};

    std::string m_name;

    virtual MaterialType getType() = 0;

private:

};

class MaterialPBR : public Material {
public:
    MaterialPBR() {};
    MaterialPBR(std::string name) : Material(name) {};

    MaterialType getType() override {
        return MaterialType::MATERIAL_PBR_STANDARD;
    }

    virtual glm::vec4& getAlbedo() = 0;

    virtual float& getMetallic() = 0;

    virtual float& getRoughness() = 0;

    virtual float& getAO() = 0;

    virtual float& getEmissive() = 0;
};

#endif
