#ifndef __SceneObject_hpp__
#define __SceneObject_hpp__

#include <vengine/math/Transform.hpp>
#include <vengine/utils/IDGeneration.hpp>
#include <vengine/utils/ECS.hpp>
#include "SceneNode.hpp"

namespace vengine
{

class Scene;

class SceneObject : public SceneNode<SceneObject>, public Entity
{
public:
    SceneObject(Scene *scene, const std::string &name, const Transform &t);
    virtual ~SceneObject();

    std::string &name() { return m_name; }
    const std::string &name() const { return m_name; }

    bool &selected() { return m_selected; }
    const bool &selected() const { return m_selected; }

    bool &active() { return m_active; }
    const bool &active() const { return m_active; }

    glm::vec3 getIDRGB() const;

    virtual void setModelMatrix(const glm::mat4 &modelMatrix) override;
    virtual void transformChanged() override;

protected:
    glm::vec3 m_idRGB;

private:
    std::string m_name = "";
    bool m_selected = false;
    bool m_active = true;

    Scene *m_scene;
};

typedef std::vector<SceneObject *> SceneGraph;

}  // namespace vengine

#endif