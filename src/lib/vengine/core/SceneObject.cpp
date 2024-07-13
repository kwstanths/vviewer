#include "SceneObject.hpp"
#include "Scene.hpp"

#include "vulkan/VulkanSceneObject.hpp"

namespace vengine
{

SceneObject::SceneObject(Scene *scene, const std::string &name, const Transform &t)
    : SceneNode(t)
    , Entity()
    , m_name(name)
    , m_scene(scene)
{
    m_idRGB = vengine::IDGeneration::toRGB(getID());
}

SceneObject::~SceneObject()
{
}

glm::vec3 SceneObject::getIDRGB() const
{
    return m_idRGB;
}

const AABB3 &SceneObject::AABB() const
{
    return m_aabb;
}

void SceneObject::computeAABB()
{
    if (has<ComponentMesh>()) {
        ComponentMesh &mc = get<ComponentMesh>();

        /* Transform mesh aabb points based on the current node model matrix */
        std::array<glm::vec3, 8> aabbPoints;
        for (uint32_t i = 0; i < 8; i++) {
            aabbPoints[i] = modelMatrix() * glm::vec4(mc.mesh->aabb().corner(i), 1);
        }

        /* Create new AABB out of all transformed corners */
        m_aabb = AABB3::fromPoint(aabbPoints[0]);
        for (uint32_t i = 1; i < 8; i++) {
            m_aabb.add(aabbPoints[i]);
        }

    } else {
        m_aabb = AABB3();
    }
}

void SceneObject::setModelMatrix(const glm::mat4 &modelMatrix)
{
    // TODO
}

void SceneObject::transformChanged()
{
    m_scene->needsUpdate(true);
}

}  // namespace vengine