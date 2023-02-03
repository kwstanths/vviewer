#ifndef __ECS_hpp__
#define __ECS_hpp__

#include <unordered_map>
#include <memory>

#include "IDGeneration.hpp"

enum class ComponentType {
    MESH = 0,
    MATERIAL = 1,
    POINT_LIGHT = 2,
};

class Component {
public:
    Component(ComponentType type);
    virtual ~Component() = default;

    ComponentType getType() const;

private:
    ComponentType m_type;
};

class Entity {
public:
    Entity();
    virtual ~Entity() = default;

    void assign(std::shared_ptr<Component> c);

    std::shared_ptr<Component> get(ComponentType type) const;

    template<typename T>
    std::shared_ptr<T> get(ComponentType type) const
    {
        return std::dynamic_pointer_cast<T>(get(type));
    }

    bool has(ComponentType type) const;

    ID getID() const;

private:
    ID m_id;
    std::unordered_map<ComponentType, std::shared_ptr<Component>> m_components;
};

#endif