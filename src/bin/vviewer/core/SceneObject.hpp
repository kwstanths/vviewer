#ifndef __SceneObject_hpp__
#define __SceneObject_hpp__

#include <math/Transform.hpp>
#include "MeshModel.hpp"
#include "Materials.hpp"

class SceneObject {
public:
    SceneObject() {};

    SceneObject(const MeshModel * meshModel);

    std::string m_name = "";

    virtual void setModelMatrix(const glm::mat4& modelMatrix);

    const MeshModel * getMeshModel() const;
    void setMeshModel(const MeshModel * newMeshModel);

    Material * getMaterial() const;
    void setMaterial(Material * newMaterial);

protected:
    const MeshModel * m_meshModel = nullptr;
    Material * m_material = nullptr;
};

#endif