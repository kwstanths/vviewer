#ifndef __SceneObject_hpp__
#define __SceneObject_hpp__

#include <vengine/math/Transform.hpp>
#include <vengine/utils/IDGeneration.hpp>
#include <vengine/utils/ECS.hpp>
#include "SceneNode.hpp"

namespace vengine
{

class SceneObject : public SceneNode<SceneObject>, public Entity
{
public:
    SceneObject(const Transform &t);
    virtual ~SceneObject();

    std::string m_name = "";

    virtual void setModelMatrix(const glm::mat4 &modelMatrix) override;

    bool m_isSelected = false;

    glm::vec3 getIDRGB() const;

protected:
    glm::vec3 m_idRGB;
};

typedef std::vector<SceneObject *> SceneGraph;

}  // namespace vengine

#endif