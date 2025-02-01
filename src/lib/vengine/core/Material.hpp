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
    MATERIAL_VOLUME = 3,
    MATERIAL_TOTAL_TYPES = 4,
};

typedef uint32_t MaterialIndex;

static const std::unordered_map<MaterialType, std::string> materialTypeNames = {{MaterialType::MATERIAL_PBR_STANDARD, "PBR standard"},
                                                                                {MaterialType::MATERIAL_SKYBOX, "Skybox"},
                                                                                {MaterialType::MATERIAL_LAMBERT, "Lambert"},
                                                                                {MaterialType::MATERIAL_VOLUME, "Homogenous Volume"}};

class Materials;
class Material : public Asset
{
public:
    Material(const AssetInfo &info, Materials &materials)
        : Asset(info)
        , m_materials(materials){};

    virtual ~Material(){};

    virtual MaterialType type() const { return MaterialType::MATERIAL_NOT_SET; };

    virtual MaterialIndex materialIndex() const { return 0; }

    virtual bool isEmissive() const { return false; }

    virtual bool isTransparent() const { return false; }
    virtual void setTransparent(bool transparent);

protected:
    Materials &m_materials;
};

class MaterialPBRStandard : public Material
{
public:
    MaterialPBRStandard(const AssetInfo &info, Materials &materials)
        : Material(info, materials){};

    virtual ~MaterialPBRStandard() {}

    MaterialType type() const override { return MaterialType::MATERIAL_PBR_STANDARD; }

    bool isEmissive() const override;

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

protected:
    Texture *m_albedoTexture = nullptr;
    Texture *m_metallicTexture = nullptr;
    Texture *m_roughnessTexture = nullptr;
    Texture *m_aoTexture = nullptr;
    Texture *m_emissiveTexture = nullptr;
    Texture *m_normalTexture = nullptr;
    Texture *m_alphaTexture = nullptr;

private:
};

class MaterialLambert : public Material
{
public:
    MaterialLambert(const AssetInfo &info, Materials &materials)
        : Material(info, materials){};

    virtual ~MaterialLambert() {}

    MaterialType type() const override { return MaterialType::MATERIAL_LAMBERT; }

    bool isEmissive() const override;

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
    MaterialSkybox(const AssetInfo &info, Materials &materials)
        : Material(info, materials){};

    virtual ~MaterialSkybox() {}

    MaterialType type() const override { return MaterialType::MATERIAL_SKYBOX; }

    virtual void setMap(EnvironmentMap *cubemap);
    EnvironmentMap *getMap() const;

protected:
    EnvironmentMap *m_envMap = nullptr;
};

class MaterialVolume : public Material
{
public:
    MaterialVolume(const AssetInfo &info, Materials &materials)
        : Material(info, materials){};

    virtual ~MaterialVolume() {}

    MaterialType type() const override { return MaterialType::MATERIAL_VOLUME; }

    virtual glm::vec4 &sigmaA() = 0;
    virtual const glm::vec4 &sigmaA() const = 0;

    virtual glm::vec4 &sigmaS() = 0;
    virtual const glm::vec4 &sigmaS() const = 0;

    /* Between (-1, 1) */
    virtual float &g() = 0;
    const virtual float &g() const = 0;

protected:
};

}  // namespace vengine

#endif
