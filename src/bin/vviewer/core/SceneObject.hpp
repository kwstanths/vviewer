#ifndef __SceneObject_hpp__
#define __SceneObject_hpp__

#include <math/Transform.hpp>
#include "MeshModel.hpp"
#include "Materials.hpp"
#include "IDGeneration.hpp"
#include "SceneGraph.hpp"

class SceneObject : public SceneNode<SceneObject> {
public:
    SceneObject(const Transform& t);
    SceneObject(const Transform& t, const Mesh * mesh);

    std::string m_name = "";

    virtual void setModelMatrix(const glm::mat4& modelMatrix) override;

    const Mesh * getMesh() const;
    void setMesh(const Mesh * newMeshModel);

    Material * getMaterial() const;
    void setMaterial(Material * newMaterial);

    bool m_isSelected = false;

    ID getID() const;
    glm::vec3 getIDRGB() const;

protected:
    const Mesh * m_mesh = nullptr;
    Material * m_material = nullptr;
    ID m_id;
    glm::vec3 m_idRGB;
};

#endif