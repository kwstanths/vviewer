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

    virtual MaterialType type() const { return MaterialType::MATERIAL_NOT_SET; };

    virtual MaterialIndex materialIndex() const { return 0; }

    bool &transparent() { return m_transparent; }
    const bool &transparent() const { return m_transparent; }

private:
    bool m_transparent = false;
};

class MaterialPBRStandard : public Material
{
public:
    MaterialPBRStandard(std::string name)
        : Material(name){};
    MaterialPBRStandard(std::string name, std::string filepath)
        : Material(name, filepath){};

    virtual ~MaterialPBRStandard() {}

    MaterialType type() const override { return MaterialType::MATERIAL_PBR_STANDARD; }

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

    virtual void setAlbedoTexture(Texture *texture);
    virtual void setMetallicTexture(Texture *texture);
    virtual void setRoughnessTexture(Texture *texture);
    virtual void setAOTexture(Texture *texture);
    virtual void setEmissiveTexture(Texture *texture);
    virtual void setNormalTexture(Texture *texture);
    virtual void setAlphaTexture(Texture *texture);

    Texture *getAlbedoTexture() const;
    Texture *getMetallicTexture() const;
    Texture *getRoughnessTexture() const;
    Texture *getAOTexture() const;
    Texture *getEmissiveTexture() const;
    Texture *getNormalTexture() const;
    Texture *getAlphaTexture() const;

    bool &zipMaterial() { return m_zipMaterial; }
    const bool &zipMaterial() const { return m_zipMaterial; }

protected:
    Texture *m_albedoTexture = nullptr;
    Texture *m_metallicTexture = nullptr;
    Texture *m_roughnessTexture = nullptr;
    Texture *m_aoTexture = nullptr;
    Texture *m_emissiveTexture = nullptr;
    Texture *m_normalTexture = nullptr;
    Texture *m_alphaTexture = nullptr;

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

    MaterialType type() const override { return MaterialType::MATERIAL_LAMBERT; }

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

    virtual void setAlbedoTexture(Texture *texture);
    virtual void setAOTexture(Texture *texture);
    virtual void setEmissiveTexture(Texture *texture);
    virtual void setNormalTexture(Texture *texture);
    virtual void setAlphaTexture(Texture *texture);

    Texture *getAlbedoTexture() const;
    Texture *getAOTexture() const;
    Texture *getEmissiveTexture() const;
    Texture *getNormalTexture() const;
    Texture *getAlphaTexture() const;

protected:
    Texture *m_albedoTexture = nullptr;
    Texture *m_aoTexture = nullptr;
    Texture *m_emissiveTexture = nullptr;
    Texture *m_normalTexture = nullptr;
    Texture *m_alphaTexture = nullptr;
};

class MaterialSkybox : public Material
{
public:
    MaterialSkybox(std::string name)
        : Material(name){};

    virtual ~MaterialSkybox() {}

    MaterialType type() const override { return MaterialType::MATERIAL_SKYBOX; }

    virtual void setMap(EnvironmentMap *cubemap);
    EnvironmentMap *getMap() const;

protected:
    EnvironmentMap *m_envMap = nullptr;
};

}  // namespace vengine

#endif
