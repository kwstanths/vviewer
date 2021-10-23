#ifndef __Material_hpp__
#define __Material_hpp__

#include <string>
#include <glm/glm.hpp>

class Material {
public:
    std::string m_name;

private:

};

class MaterialPBR : public Material {
public:
    virtual glm::vec4& getAlbedo() = 0;

    virtual float& getMetallic() = 0;

    virtual float& getRoughness() = 0;

    virtual float& getAO() = 0;

    virtual float& getEmissive() = 0;
};

#endif
