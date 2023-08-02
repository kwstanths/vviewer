#include "ECS.hpp"

Entity::Entity() 
{
    m_id = IDGeneration::getInstance().getID();
};

ID Entity::getID() const
{
    return m_id;
}

void ComponentManager::clear()
{
    for (auto& it: m_componentBuffers) {
        it.second->clear();
    }
}
