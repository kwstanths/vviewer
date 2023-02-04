#ifndef __ECS_hpp__
#define __ECS_hpp__

#include <unordered_map>
#include <memory>
#include <stdexcept>

#include <utils/Console.hpp>

#include "IDGeneration.hpp"
#include "FreeList.hpp"

static const uint32_t MAX_COMPONENTS = 1000;

class Mesh;
class Material;
class PointLight;

class Component {
public:
    virtual ~Component() = default;
private:
};

class ComponentMesh : public Component {
public:
    ComponentMesh() {};
    ComponentMesh(const std::shared_ptr<Mesh>& m) : mesh(m) {};
    std::shared_ptr<Mesh> mesh;
};
class ComponentMaterial : public Component {
public:
    ComponentMaterial() {};
    ComponentMaterial(const std::shared_ptr<Material>& m) : material(m) {};
    std::shared_ptr<Material> material;
};
class ComponentPointLight : public Component {
public:
    ComponentPointLight() {};
    ComponentPointLight(const std::shared_ptr<PointLight>& l) : light(l) {};
    std::shared_ptr<PointLight> light;
};

class ComponentManager {
public:
    static ComponentManager& getInstance()
    {
        static ComponentManager instance;
        return instance;
    }
    ComponentManager(ComponentManager const&) = delete;
    void operator=(ComponentManager const&) = delete;

    /* Create a component */
    template<typename T, typename... Args>
    T* create(Args... args);

    /* Remove a component */
    template<typename T>
    void remove(T * t);

    void clear();

private:
    ComponentManager() : m_meshes(MAX_COMPONENTS), m_materials(MAX_COMPONENTS), m_pointLights(MAX_COMPONENTS) {};

    FreeBlockList<ComponentMesh> m_meshes;
    FreeBlockList<ComponentMaterial> m_materials;
    FreeBlockList<ComponentPointLight> m_pointLights;
};

class Entity {
public:
    Entity();
    virtual ~Entity()
    {
        // TODO optimise this 
        remove<ComponentMesh>();
        remove<ComponentMaterial>();
        remove<ComponentPointLight>();
    }

    /* c pointers MUST come from the component manager */
    void assign(Component * c)
    {
        const char * name = typeid(*c).name();
        if (m_components.find(name) != m_components.end())
        {
            utils::ConsoleWarning("assign(): Assigning a component to entity that already has that component");
        }

        m_components[name] = c;
    }

    template<typename T>
    T& get() const
    {
        static_assert(std::is_base_of<Component, T>::value);

        const char * name = typeid(T).name();
        auto itr = m_components.find(name);
        if (itr == m_components.end())
        {
            throw std::runtime_error("Entity::get(): Component doesn't exist");
        }
        else return *static_cast<T*>(itr->second);
    }

    template<typename T>
    bool has() const
    {
        static_assert(std::is_base_of<Component, T>::value);

        const char * name = typeid(T).name();
        return m_components.find(name) != m_components.end();
    }

    ID getID() const;

    template<typename T>
    void remove()
    {
        static_assert(std::is_base_of<Component, T>::value);

        if (!has<T>()) return;
        
        const char * name = typeid(T).name();
        T * t = static_cast<T*>(m_components[name]);
        m_components.erase(name);

        auto& cm = ComponentManager::getInstance();
        cm.remove<T>(t);
    }

private:
    ID m_id;
    std::unordered_map<const char *, Component*> m_components;
};


#endif