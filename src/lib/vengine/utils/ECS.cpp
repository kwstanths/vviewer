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
    assert(mesh != nullptr);
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
    assert(material != nullptr);

    /* Use ComponentVolume to create volumes */
    if (material->type() == MaterialType::MATERIAL_VOLUME)
        return;

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
    assert(light != nullptr);
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

ComponentVolume::ComponentVolume(MaterialVolume *frontFacing, MaterialVolume *backFacing)
{
    m_materialFrontFacing = frontFacing;
    m_materialBackFacing = backFacing;
}

MaterialVolume *ComponentVolume::frontFacing() const
{
    return m_materialFrontFacing;
}

MaterialVolume *ComponentVolume::backFacing() const
{
    return m_materialBackFacing;
}

void ComponentVolume::setFrontFacingVolume(MaterialVolume *frontFacing)
{
    m_materialFrontFacing = frontFacing;

    for (Entity *e : *m_owner) {
        e->onVolumeComponentChanged();
    }
}

void ComponentVolume::setBackFacingVolume(MaterialVolume *backFacing)
{
    m_materialBackFacing = backFacing;

    for (Entity *e : *m_owner) {
        e->onVolumeComponentChanged();
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