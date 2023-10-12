#ifndef __Material_hpp__
#define __Material_hpp__

#include <string>
#include <unordered_map>
#include <memory>

#include <glm/glm.hpp>

#include "Asset.hpp"
#include "Texture.hpp"
#include "Cubemap.hpp"
#include "EnvironmentMap.hpp"

namespace vengine
{

enum class MaterialType {
    MATERIAL_NOT_SET = -1,
    MATERIAL_PBR_STANDARD = 0,
    MATERIAL_SKYBOX = 1,
    MATERIAL_LAMBERT = 2,
};

typedef uint32_t MaterialIndex;

static const std::unordered_map<MaterialType, std::string> materialTypeNames = {
    {MaterialType::MATERIAL_PBR_STANDARD, "PBR standard"},
    {MaterialType::MATERIAL_SKYBOX, "Skybox"},
    {MaterialType::MATERIAL_LAMBERT, "Lambert"},
};

class Material : public Asset
{
public:
    Material(std::string name)
        : Asset(name){};
    Material(std::string name, std::string filepath)
        : Asset(name, filepath){};

    virtual ~Material(){};

    virtual MaterialType getType() const { return MaterialType::MATERIAL_NOT_SET; };

    virtual MaterialIndex getMaterialIndex() const { return 0; }

private:
};

class MaterialPBRStandard : public Material
{
public:
    MaterialPBRStandard(std::string name)
        : Material(name){};
    MaterialPBRStandard(std::string name, std::string filepath)
        : Material(name, filepath){};

    virtual ~MaterialPBRStandard() {}

    MaterialType getType() const override { return MaterialType::MATERIAL_PBR_STANDARD; }

    virtual glm::vec4 &albedo() = 0;
    virtual const glm::vec4 &albedo() const = 0;
    virtual float &metallic() = 0;
    virtual const float &metallic() const = 0;
    virtual float &roughness() = 0;
    virtual const float &roughness() const = 0;
    virtual float &ao() = 0;
    virtual const float &ao() const = 0;
    virtual glm::vec4 &emissive() = 0;
    virtual const glm::vec4 &emissive() const = 0;
    virtual float &emissiveIntensity() = 0;
    virtual const float &emissiveIntensity() const = 0;
    virtual glm::vec3 emissiveColor() const = 0;
    virtual float &uTiling() = 0;
    virtual const float &uTiling() const = 0;
    virtual float &vTiling() = 0;
    virtual const float &vTiling() const = 0;

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

    bool &zipMaterial() { return m_zipMaterial; }
    const bool &zipMaterial() const { return m_zipMaterial; }

protected:
    std::shared_ptr<Texture> m_albedoTexture;
    std::shared_ptr<Texture> m_metallicTexture;
    std::shared_ptr<Texture> m_roughnessTexture;
    std::shared_ptr<Texture> m_aoTexture;
    std::shared_ptr<Texture> m_emissiveTexture;
    std::shared_ptr<Texture> m_normalTexture;

private:
    bool m_zipMaterial = false;
};

class MaterialLambert : public Material
{
public:
    MaterialLambert(std::string name)
        : Material(name){};
    MaterialLambert(std::string name, std::string filepath)
        : Material(name, filepath){};

    virtual ~MaterialLambert() {}

    MaterialType getType() const override { return MaterialType::MATERIAL_LAMBERT; }

    virtual glm::vec4 &albedo() = 0;
    virtual const glm::vec4 &albedo() const = 0;
    virtual float &ao() = 0;
    virtual const float &ao() const = 0;
    virtual glm::vec4 &emissive() = 0;
    virtual const glm::vec4 &emissive() const = 0;
    virtual float &emissiveIntensity() = 0;
    virtual const float &emissiveIntensity() const = 0;
    virtual glm::vec3 emissiveColor() const = 0;
    virtual float &uTiling() = 0;
    virtual const float &uTiling() const = 0;
    virtual float &vTiling() = 0;
    virtual const float &vTiling() const = 0;

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

class MaterialSkybox : public Material
{
public:
    MaterialSkybox(std::string name)
        : Material(name){};

    virtual ~MaterialSkybox() {}

    MaterialType getType() const override { return MaterialType::MATERIAL_SKYBOX; }

    virtual void setMap(std::shared_ptr<EnvironmentMap> cubemap);
    std::shared_ptr<EnvironmentMap> getMap() const;

protected:
    std::shared_ptr<EnvironmentMap> m_envMap = nullptr;
};

}  // namespace vengine

#endif
