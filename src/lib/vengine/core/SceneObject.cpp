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
}

SceneObject::~SceneObject()
{
}

void SceneObject::setActive(bool active)
{
    m_active = active;
    m_scene->invalidateInstances(true);
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
            aabbPoints[i] = modelMatrix() * glm::vec4(mc.mesh()->aabb().corner(i), 1);
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

void SceneObject::transformChanged()
{
    m_scene->invalidateSceneGraph(true);
}

void SceneObject::updateModelMatrix(const glm::mat4 &modelMatrix)
{
    InstanceData *instanceData = m_scene->instancesManager().findInstanceData(this);
    if (instanceData != nullptr) {
        instanceData->modelMatrix = modelMatrix;
    }
    computeAABB();
}

void SceneObject::onComponentAdded()
{
    m_scene->invalidateInstances(true);
}

void SceneObject::onComponentRemoved()
{
    m_scene->invalidateInstances(true);
}

void SceneObject::onMeshComponentChanged()
{
    m_scene->invalidateInstances(true);
}

void SceneObject::onMaterialComponentChanged()
{
    m_scene->invalidateInstances(true);
}

void SceneObject::onLightComponentChanged()
{
    /* don't have to invalidate scene, light instance data are built every frame regardless the invalidation of isntances */
    // m_scene->invalidateInstances(true);
}

}  // namespace vengine