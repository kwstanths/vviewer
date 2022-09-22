#include "ECS.hpp"

Component::Component(ComponentType type) : m_type(type) {

};

ComponentType Component::getType() const
{
    return m_type;
}

Entity::Entity() 
{
    m_id = IDGeneration::getInstance().getID();
};

void Entity::assign(Component *c)
{
    m_components[c->getType()] = c;
}

Component * Entity::get(ComponentType type) const
{
    auto itr = m_components.find(type);
    if (itr == m_components.end()) return nullptr;
    else return itr->second;
}

bool Entity::has(ComponentType type) const
{
    return m_components.find(type) != m_components.end();
}

ID Entity::getID() const
{
    return m_id;
}
