#ifndef __Material_hpp__
#define __Material_hpp__

#include <string>
#include <unordered_map>
#include <memory>

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

class Material {
public:
    Material() {};
    Material(std::string name): m_name(name) {};

    std::string m_name;

    /* If it's a zip stack material */
    std::string m_path = "";

    virtual MaterialType getType() const { return MaterialType::MATERIAL_NOT_SET; };

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

    virtual void setAlbedoTexture(std::shared_ptr<Texture> texture);
    virtual void setMetallicTexture(std::shared_ptr<Texture> texture);
    virtual void setRoughnessTexture(std::shared_ptr<Texture> texture);
    virtual void setAOTexture(std::shared_ptr<Texture> texture);
    virtual void setEmissiveTexture(std::shared_ptr<Texture> texture);
    virtual void setNormalTexture(std::shared_ptr<Texture> texture);

    std::shared_ptr<Texture> getAlbedoTexture() const;
    std::shared_ptr<Texture> getMetallicTexture() const;
    std::shared_ptr<Texture> getRoughnessTexture() const;
    std::shared_ptr<Texture> getAOTexture() const;
    std::shared_ptr<Texture> getEmissiveTexture() const;
    std::shared_ptr<Texture> getNormalTexture() const;

protected:
    std::shared_ptr<Texture> m_albedoTexture;
    std::shared_ptr<Texture> m_metallicTexture;
    std::shared_ptr<Texture> m_roughnessTexture;
    std::shared_ptr<Texture> m_aoTexture;
    std::shared_ptr<Texture> m_emissiveTexture;
    std::shared_ptr<Texture> m_normalTexture;
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

    virtual void setAlbedoTexture(std::shared_ptr<Texture> texture);
    virtual void setAOTexture(std::shared_ptr<Texture> texture);
    virtual void setEmissiveTexture(std::shared_ptr<Texture> texture);
    virtual void setNormalTexture(std::shared_ptr<Texture> texture);

    std::shared_ptr<Texture> getAlbedoTexture() const;
    std::shared_ptr<Texture> getAOTexture() const;
    std::shared_ptr<Texture> getEmissiveTexture() const;
    std::shared_ptr<Texture> getNormalTexture() const;

protected:
    std::shared_ptr<Texture> m_albedoTexture;
    std::shared_ptr<Texture> m_aoTexture;
    std::shared_ptr<Texture> m_emissiveTexture;
    std::shared_ptr<Texture> m_normalTexture;
};

class MaterialSkybox : public Material {
public:
    MaterialSkybox() {};
    MaterialSkybox(std::string name) : Material(name) {};

    MaterialType getType() const override {
        return MaterialType::MATERIAL_SKYBOX;
    }

    virtual void setMap(std::shared_ptr<EnvironmentMap> cubemap);
    std::shared_ptr<EnvironmentMap> getMap() const;

protected:
    std::shared_ptr<EnvironmentMap> m_envMap = nullptr;
};

#endif
