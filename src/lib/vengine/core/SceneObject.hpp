#ifndef __SceneObject_hpp__
#define __SceneObject_hpp__

#include <vengine/math/Transform.hpp>
#include <vengine/math/AABB.hpp>
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

    void setActive(bool active);
    bool isActive() const { return m_active; }

    const AABB3 &AABB() const;

    void computeAABB();

private:
    std::string m_name = "";
    bool m_selected = false;
    bool m_active = true;

    AABB3 m_aabb;

    Scene *m_scene = nullptr;

    virtual void transformChanged() override;
    virtual void updateModelMatrix(const glm::mat4 &modelMatrix) override;
    virtual void onComponentAdded() override;
    virtual void onComponentRemoved() override;
    virtual void onMeshComponentChanged() override;
    virtual void onMaterialComponentChanged() override;
    virtual void onLightComponentChanged() override;
    virtual void onVolumeComponentChanged() override;
};

typedef std::vector<SceneObject *> SceneObjectVector;

}  // namespace vengine

#endif