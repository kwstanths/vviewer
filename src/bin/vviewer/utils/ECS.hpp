#ifndef __ECS_hpp__
#define __ECS_hpp__

#include <unordered_map>

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

    void assign(Component *c);

    Component * get(ComponentType type) const;

    template<typename T>
    T get(ComponentType type) const
    {
        return dynamic_cast<T>(get(type));
    }

    bool has(ComponentType type) const;

    ID getID() const;

private:
    ID m_id;
    std::unordered_map<ComponentType, Component *> m_components;
};

#endif