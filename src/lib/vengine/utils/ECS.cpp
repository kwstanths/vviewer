#include "ECS.hpp"

namespace vengine
{

ComponentMesh::ComponentMesh(Mesh *mesh)
    : m_mesh(mesh)
{
}

Mesh *ComponentMesh::mesh() const
{
    return m_mesh;
}

void ComponentMesh::setMesh(Mesh *mesh)
{
    m_mesh = mesh;

    for (Entity *e : *m_owner) {
        e->onMeshComponentChanged();
    }
}

ComponentMaterial::ComponentMaterial(Material *material)
    : m_material(material)
{
}

Material *ComponentMaterial::material() const
{
    return m_material;
}

void ComponentMaterial::setMaterial(Material *material)
{
    m_material = material;

    for (Entity *e : *m_owner) {
        e->onMaterialComponentChanged();
    }
}

ComponentLight::ComponentLight(Light *light)
    : m_light(light)
{
}

Light *ComponentLight::light() const
{
    return m_light;
}

void ComponentLight::setLight(Light *light)
{
    m_light = light;

    for (Entity *e : *m_owner) {
        e->onLightComponentChanged();
    }
}

bool ComponentLight::castShadows()
{
    return m_castShadows;
}

void ComponentLight::setCastShadows(bool castShadows)
{
    m_castShadows = castShadows;

    for (Entity *e : *m_owner) {
        e->onLightComponentChanged();
    }
}

ComponentOwner *Component::owner() const
{
    return m_owner;
}

Entity::Entity()
{
    m_id = IDGeneration::getInstance().generate();
};

ID Entity::getID() const
{
    return m_id;
}

void ComponentManager::clear()
{
    for (auto &it : m_componentBuffers) {
        it.second->clear();
    }
}

}  // namespace vengine