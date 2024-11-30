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

static const uint32_t MAX_COMPONENTS = 32768;

class Entity;
class ComponentOwner;
class ComponentManager;

/* Base Component class */
class Component
{
    friend class Entity;
    friend class ComponentManager;

public:
    virtual ~Component() = default;

    ComponentOwner *owner() const;

protected:
    ComponentOwner *m_owner = nullptr;
};
/* Available components */
class ComponentMesh : public Component
{
public:
    ComponentMesh(){};
    ComponentMesh(Mesh *mesh);
    Mesh *mesh() const;
    void setMesh(Mesh *mesh);

private:
    Mesh *m_mesh = nullptr;
};
class ComponentMaterial : public Component
{
public:
    ComponentMaterial(){};
    ComponentMaterial(Material *material);
    Material *material() const;
    void setMaterial(Material *material);

private:
    Material *m_material = nullptr;
};
class ComponentLight : public Component
{
public:
    ComponentLight(){};
    ComponentLight(Light *light);
    Light *light() const;
    void setLight(Light *light);

    bool castShadows();
    void setCastShadows(bool castShadows);

private:
    Light *m_light = nullptr;
    bool m_castShadows = true;
};

/* Component owner */
class ComponentOwner
{
    friend class Entity;

public:
    virtual ~ComponentOwner() = default;
    struct Iterator {
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using pointer = Entity **;
        using reference = Entity *&;

        Iterator(pointer ptr)
            : m_ptr(ptr)
        {
        }

        reference operator*() const { return *m_ptr; }
        pointer operator->() { return m_ptr; }
        Iterator &operator++()
        {
            m_ptr++;
            return *this;
        }
        Iterator operator++(int)
        {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }
        friend bool operator==(const Iterator &a, const Iterator &b) { return a.m_ptr == b.m_ptr; };
        friend bool operator!=(const Iterator &a, const Iterator &b) { return a.m_ptr != b.m_ptr; };

        pointer m_ptr;
    };

    virtual bool isShared() = 0;

    virtual Iterator begin() = 0;
    virtual Iterator end() = 0;
};
class ComponentOwnerShared : public ComponentOwner
{
    friend class Entity;

public:
    bool isShared() { return true; }
    Iterator begin()
    {
        if (m_entities.size() == 0)
            return nullptr;
        return &m_entities[0];
    }
    Iterator end()
    {
        if (m_entities.size() == 0)
            return nullptr;
        return &m_entities[m_entities.size() - 1] + 1;
    }

private:
    void addEntity(Entity *e) { m_entities.push_back(e); }
    std::vector<Entity *> m_entities;
};
class ComponentOwnerUnique : public ComponentOwner
{
    friend class Entity;

public:
    bool isShared() { return false; }
    Iterator begin() { return &m_entity; }
    Iterator end() { return &m_entity + 1; }

private:
    Entity *m_entity = nullptr;
};

/* Component storage buffers */
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

    template <typename T, typename OwnerType = ComponentOwnerUnique>
    T *create()
    {
        static_assert(std::is_base_of<Component, T>::value);
        static_assert(std::is_base_of<ComponentOwner, OwnerType>::value);

        auto componentBuffer = buffer<T>();
        auto *component = componentBuffer->get();
        component->m_owner = new OwnerType();
        return component;
    }

    template <typename T>
    void remove(T *t)
    {
        static_assert(std::is_base_of<Component, T>::value);

        delete t->m_owner;
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

/* Entity class */
class Entity
{
public:
    Entity();

    virtual ~Entity()
    {
        // TODO optimise this?
        remove<ComponentMesh>();
        remove<ComponentMaterial>();
        remove<ComponentLight>();
    }

    /**
     * @brief Creates a new unique component of type T, adds to the entity and returns its reference
     *
     * @tparam T
     * @return T&
     */
    template <typename T>
    T &add()
    {
        static_assert(std::is_base_of<Component, T>::value);

        if (has<T>())
            return get<T>();

        auto &cm = ComponentManager::getInstance();
        T *c = cm.create<T, ComponentOwnerUnique>();
        static_cast<ComponentOwnerUnique *>(c->m_owner)->m_entity = this;

        const char *name = typeid(T).name();
        m_components[name] = c;

        onComponentAdded();

        return *c;
    }

    /**
     * @brief Adds a shared component of type T to the entity and returns its reference
     *
     * @tparam T
     * @return T&
     */
    template <typename T>
    void add_shared(Component *sharedComponent)
    {
        static_assert(std::is_base_of<Component, T>::value);
        assert(sharedComponent != nullptr);
        assert(sharedComponent->m_owner != nullptr);
        assert(sharedComponent->m_owner->isShared());

        if (has<T>())
            return;

        static_cast<ComponentOwnerShared *>(sharedComponent->m_owner)->addEntity(this);

        const char *name = typeid(T).name();
        m_components[name] = sharedComponent;

        onComponentAdded();
    }

    /**
     * @brief Get the component of type T
     *
     * @return Get&
     */
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

    /**
     * @brief Check if it has the component of type T
     *
     * @tparam T
     * @return true
     * @return false
     */
    template <typename T>
    bool has() const
    {
        static_assert(std::is_base_of<Component, T>::value);

        const char *name = typeid(T).name();
        auto itr = m_components.find(name);
        return (itr != m_components.end());
    }

    ID getID() const;

    /**
     * @brief Remove a component of type T
     *
     * @tparam T
     */
    template <typename T>
    void remove()
    {
        static_assert(std::is_base_of<Component, T>::value);

        if (!has<T>())
            return;

        const char *name = typeid(T).name();
        T *t = static_cast<T *>(m_components[name]);
        m_components.erase(name);

        if (!t->m_owner->isShared()) {
            auto &cm = ComponentManager::getInstance();
            cm.remove<T>(t);
        }

        onComponentRemoved();
    }

    virtual void onComponentAdded() {}
    virtual void onComponentRemoved() {}
    virtual void onMeshComponentChanged() {}
    virtual void onMaterialComponentChanged() {}
    virtual void onLightComponentChanged() {}

private:
    ID m_id;
    std::unordered_map<const char *, Component *> m_components;
};

}  // namespace vengine

#endif