#include "ECS.hpp"

Entity::Entity() 
{
    m_id = IDGeneration::getInstance().getID();
};

ID Entity::getID() const
{
    return m_id;
}

template<>
ComponentMesh* ComponentManager::create<ComponentMesh, std::shared_ptr<Mesh>>(std::shared_ptr<Mesh> m) {
    return m_meshes.add(ComponentMesh(m));
}
template<>
ComponentMaterial* ComponentManager::create<ComponentMaterial, std::shared_ptr<Material>>(std::shared_ptr<Material> m) {
    return m_materials.add(ComponentMaterial(m));
}
template<>
ComponentPointLight* ComponentManager::create<ComponentPointLight, std::shared_ptr<PointLight>>(std::shared_ptr<PointLight> pl) {
    return m_pointLights.add(ComponentPointLight(pl));
}

template<>
void ComponentManager::remove<ComponentMesh>(ComponentMesh * m) {
    size_t index = m_meshes.getIndex(m);
    m_meshes.remove(index);
}
template<>
void ComponentManager::remove<ComponentMaterial>(ComponentMaterial * m) {
    size_t index = m_materials.getIndex(m);
    m_materials.remove(index);
}
template<>
void ComponentManager::remove<ComponentPointLight>(ComponentPointLight * m) {
    size_t index = m_pointLights.getIndex(m);
    m_pointLights.remove(index);
}

void ComponentManager::clear()
{
    m_meshes.clear();
    m_materials.clear();
    m_pointLights.clear();
}
