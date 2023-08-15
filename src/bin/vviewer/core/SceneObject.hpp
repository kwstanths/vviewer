#ifndef __SceneObject_hpp__
#define __SceneObject_hpp__

#include <math/Transform.hpp>
#include <utils/IDGeneration.hpp>
#include <utils/ECS.hpp>
#include "MeshModel.hpp"
#include "Materials.hpp"
#include "SceneNode.hpp"

class SceneObject : public SceneNode<SceneObject> , public Entity {
public:
    SceneObject(const Transform& t);
    virtual ~SceneObject();

    std::string m_name = "";

    virtual void setModelMatrix(const glm::mat4& modelMatrix) override;

    bool m_isSelected = false;

    glm::vec3 getIDRGB() const;

protected:
    glm::vec3 m_idRGB;
};

#endif