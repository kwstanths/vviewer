#ifndef __Material_hpp__
#define __Material_hpp__

#include <string>
#include <unordered_map>

#include <glm/glm.hpp>

#include <utils/ECS.hpp>
#include "Texture.hpp"
#include "Cubemap.hpp"
#include "EnvironmentMap.hpp"

enum class MaterialType {
    MATERIAL_NOT_SET = -1,
    MATERIAL_PBR_STANDARD = 0,
    MATERIAL_SKYBOX = 1,
    MATERIAL_LAMBERT = 2,
};

static const std::unordered_map<MaterialType, std::string> materialTypeNames = {
    { MaterialType::MATERIAL_PBR_STANDARD, "PBR standard" },
    { MaterialType::MATERIAL_SKYBOX, "Skybox" },
    { MaterialType::MATERIAL_LAMBERT, "Lambert" },
};

class Material : public Component {
public:
    Material() : Component(ComponentType::MATERIAL) {};
    Material(std::string name) : Component(ComponentType::MATERIAL), m_name(name) {};

    std::string m_name;

    virtual MaterialType getType() const = 0;

private:

};

class MaterialPBRStandard : public Material {
public:
    MaterialPBRStandard() {};
    MaterialPBRStandard(std::string name) : Material(name) {};

    MaterialType getType() const override {
        return MaterialType::MATERIAL_PBR_STANDARD;
    }

    virtual glm::vec4& albedo() = 0;
    virtual glm::vec4 getAlbedo() const = 0;
    virtual float& metallic() = 0;
    virtual float getMetallic() const = 0;
    virtual float& roughness() = 0;
    virtual float getRoughness() const = 0;
    virtual float& ao() = 0;
    virtual float getAO() const = 0;
    virtual float& emissive() = 0;
    virtual float getEmissive() const = 0;
    virtual float& uTiling() = 0;
    virtual float getUTiling() const = 0;
    virtual float& vTiling() = 0;
    virtual float getVTiling() const = 0;

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

class MaterialLambert : public Material {
public:
    MaterialLambert() {};
    MaterialLambert(std::string name) : Material(name) {};

    MaterialType getType() const override {
        return MaterialType::MATERIAL_LAMBERT;
    }

    virtual glm::vec4& albedo() = 0;
    virtual glm::vec4 getAlbedo() const = 0;
    virtual float& ao() = 0;
    virtual float getAO() const = 0;
    virtual float& emissive() = 0;
    virtual float getEmissive() const = 0;
    virtual float& uTiling() = 0;
    virtual float getUTiling() const = 0;
    virtual float& vTiling() = 0;
    virtual float getVTiling() const = 0;

    virtual void setAlbedoTexture(Texture* texture);
    virtual void setAOTexture(Texture* texture);
    virtual void setEmissiveTexture(Texture* texture);
    virtual void setNormalTexture(Texture* texture);

    Texture* getAlbedoTexture() const;
    Texture* getAOTexture() const;
    Texture* getEmissiveTexture() const;
    Texture* getNormalTexture() const;

protected:
    Texture* m_albedoTexture = nullptr;
    Texture* m_aoTexture = nullptr;
    Texture* m_emissiveTexture = nullptr;
    Texture* m_normalTexture = nullptr;
};

class MaterialSkybox : public Material {
public:
    MaterialSkybox() {};
    MaterialSkybox(std::string name) : Material(name) {};

    MaterialType getType() const override {
        return MaterialType::MATERIAL_SKYBOX;
    }

    virtual void setMap(EnvironmentMap* cubemap);
    EnvironmentMap* getMap() const;

protected:
    EnvironmentMap* m_envMap = nullptr;
};

#endif
