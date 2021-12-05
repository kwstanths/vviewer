#ifndef __Material_hpp__
#define __Material_hpp__

#include <string>
#include <unordered_map>

#include <glm/glm.hpp>

#include "Texture.hpp"
#include "Cubemap.hpp"

enum class MaterialType {
    MATERIAL_NOT_SET = -1,
    MATERIAL_PBR_STANDARD = 0,
    MATERIAL_SKYBOX = 1,
};

static const std::unordered_map<MaterialType, std::string> materialTypeNames = {
    { MaterialType::MATERIAL_PBR_STANDARD, "PBR standard" },
    { MaterialType::MATERIAL_SKYBOX, "Skybox" }
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

    virtual void setAlbedoTexture(Texture * texture);
    virtual void setMetallicTexture(Texture * texture);
    virtual void setRoughnessTexture(Texture * texture);
    virtual void setAOTexture(Texture * texture);
    virtual void setEmissiveTexture(Texture * texture);
    virtual void setNormalTexture(Texture * texture);

    Texture * getAlbedoTexture() const;
    Texture * getMetallicTexture() const;
    Texture * getRoughnessTexture() const;
    Texture * getAOTexture() const;
    Texture * getEmissiveTexture() const;
    Texture * getNormalTexture() const;

protected:
    Texture * m_albedoTexture = nullptr;
    Texture * m_metallicTexture = nullptr;
    Texture * m_roughnessTexture = nullptr;
    Texture * m_aoTexture = nullptr;
    Texture * m_emissiveTexture = nullptr;
    Texture * m_normalTexture = nullptr;
};

class MaterialSkybox : public Material {
public:
    MaterialSkybox() {};
    MaterialSkybox(std::string name) : Material(name) {};

    MaterialType getType() override {
        return MaterialType::MATERIAL_SKYBOX;
    }

    virtual void setMap(Cubemap * cubemap);

protected:
    Cubemap * m_cubemap = nullptr;
};

#endif
