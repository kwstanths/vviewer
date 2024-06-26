#ifndef __ECS_hpp__
#define __ECS_hpp__

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <memory>
#include <stdexcept>

#include <debug_tools/Console.hpp>

#include "core/Mesh.hpp"
#include "core/Material.hpp"
#include "core/Light.hpp"

#include "IDGeneration.hpp"
#include "FreeList.hpp"

namespace vengine
{

static const uint32_t MAX_COMPONENTS = 16384;

class Entity;

/* Base Component class */
class Component
{
    friend class Entity;

public:
    virtual ~Component() = default;

    Entity *entity() const;

private:
    Entity *m_entity = nullptr;
};

class ComponentMesh : public Component
{
public:
    Mesh *mesh;
};
class ComponentMaterial : public Component
{
public:
    Material *material;
};
class ComponentLight : public Component
{
public:
    Light *light;
    bool castShadows = true;
};

/* Component buffers */
class IComponentBuffer
{
public:
    virtual ~IComponentBuffer(){};

    virtual void clear() = 0;

private:
};
template <typename T>
class ComponentBuffer : public IComponentBuffer, public FreeBlockList<T>
{
public:
    ComponentBuffer(uint32_t nComponents)
        : FreeBlockList<T>(nComponents){};

    void clear() override { FreeBlockList<T>::reset(); }

private:
};

/* Component manager */
class ComponentManager
{
    friend class Entity;

public:
    static ComponentManager &getInstance()
    {
        static ComponentManager instance;
        return instance;
    }
    ComponentManager(ComponentManager const &) = delete;
    void operator=(ComponentManager const &) = delete;

    void clear();

    template <typename T>
    T *create()
    {
        auto componentBuffer = buffer<T>();
        auto *component = componentBuffer->get();
        return component;
    }

    template <typename T>
    void remove(T *t)
    {
        auto componentBuffer = buffer<T>();
        componentBuffer->remove(t);
    }

    template <typename T>
    ComponentBuffer<T> *buffer()
    {
        const char *name = typeid(T).name();
        return static_cast<ComponentBuffer<T> *>(m_componentBuffers[name]);
    }

private:
    ComponentManager()
    {
        {
            const char *name = typeid(ComponentMesh).name();
            m_componentBuffers.insert({name, new ComponentBuffer<ComponentMesh>(MAX_COMPONENTS)});
        }
        {
            const char *name = typeid(ComponentMaterial).name();
            m_componentBuffers.insert({name, new ComponentBuffer<ComponentMaterial>(MAX_COMPONENTS)});
        }
        {
            const char *name = typeid(ComponentLight).name();
            m_componentBuffers.insert({name, new ComponentBuffer<ComponentLight>(MAX_COMPONENTS)});
        }
    }
    ~ComponentManager()
    {
        for (auto &itr : m_componentBuffers) {
            delete itr.second;
        }
    }
    std::unordered_map<const char *, IComponentBuffer *> m_componentBuffers;
};

/* Entity */
class Entity
{
public:
    Entity();

    virtual ~Entity()
    {
        // TODO optimise this
        remove<ComponentMesh>();
        remove<ComponentMaterial>();
        remove<ComponentLight>();
    }

    template <typename T>
    T &add()
    {
        static_assert(std::is_base_of<Component, T>::value);

        if (has<T>())
            return get<T>();

        auto &cm = ComponentManager::getInstance();
        T *c = cm.create<T>();
        c->m_entity = this;

        const char *name = typeid(T).name();
        m_components[name] = c;
        return *c;
    }

    template <typename T>
    T &get() const
    {
        static_assert(std::is_base_of<Component, T>::value);

        const char *name = typeid(T).name();
        auto itr = m_components.find(name);
        if (itr == m_components.end()) {
            throw std::runtime_error("Entity::get(): Component doesn't exist");
        } else
            return *static_cast<T *>(itr->second);
    }

    template <typename T>
    bool has() const
    {
        static_assert(std::is_base_of<Component, T>::value);

        const char *name = typeid(T).name();
        auto itr = m_components.find(name);
        return (itr != m_components.end());
    }

    ID getID() const;

    template <typename T>
    void remove()
    {
        static_assert(std::is_base_of<Component, T>::value);

        if (!has<T>())
            return;

        const char *name = typeid(T).name();
        T *t = static_cast<T *>(m_components[name]);
        m_components.erase(name);

        auto &cm = ComponentManager::getInstance();
        cm.remove<T>(t);
    }

private:
    ID m_id;
    std::unordered_map<const char *, Component *> m_components;
};

}  // namespace vengine

#endif