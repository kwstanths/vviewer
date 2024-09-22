#ifndef __Scene_hpp__
#define __Scene_hpp__

#include <memory>
#include <utility>

#include <vengine/utils/IDGeneration.hpp>

#include "Camera.hpp"
#include "Light.hpp"
#include "SceneObject.hpp"
#include "Instances.hpp"
#include "io/Export.hpp"
#include "utils/ECS.hpp"

namespace vengine
{

enum class EnvironmentType {
    /* Order matters for the UI */
    SOLID_COLOR = 0,
    HDRI = 1,
    SOLID_COLOR_WITH_HDRI_LIGHTING = 2
};

/* A clone of the GPU struct */
struct SceneData {
    glm::mat4 m_view;
    glm::mat4 m_viewInverse;
    glm::mat4 m_projection;
    glm::mat4 m_projectionInverse;
    glm::vec4 m_exposure;   /* R = exposure, G = environment map intensity, B = lens radius, A = focal distance */
    glm::vec4 m_background; /* RGB = background color, A = environment type */
    glm::vec4 m_volumes;    /* R = material id of camera volume, G = near plane, B = far plane, A = unused */
};

class Engine;

class Scene
{
    friend class SceneObject;
    friend class InstancesManager;
    friend class Materials;

public:
    Scene(Engine &engine);
    ~Scene();

    const std::shared_ptr<Camera> &camera() const { return m_camera; }
    std::shared_ptr<Camera> &camera() { return m_camera; }

    const float &exposure() const { return m_exposure; }
    float &exposure() { return m_exposure; }

    const float &environmentIntensity() const { return m_environmentIntensity; }
    float &environmentIntensity() { return m_environmentIntensity; }

    const MaterialSkybox *skyboxMaterial() const { return m_skybox; }
    MaterialSkybox *&skyboxMaterial() { return m_skybox; }

    const EnvironmentType &environmentType() const { return m_environmentType; }
    EnvironmentType &environmentType() { return m_environmentType; }

    const glm::vec3 &backgroundColor() const { return m_backgroundColor; }
    glm::vec3 &backgroundColor() { return m_backgroundColor; }

    virtual SceneData getSceneData() const;

    /* Add a new scene object at the root of the scene graph */
    SceneObject *addSceneObject(std::string name, Transform transform);
    /* Add a new scene object as a child of a node */
    SceneObject *addSceneObject(std::string name, SceneObject *parentNode, Transform transform);

    void removeSceneObject(SceneObject *object);

    virtual void update();

    SceneObjectVector &sceneGraph();
    /* Get all scene objects in a flat array */
    SceneObjectVector getSceneObjectsFlat() const;

    SceneObject *findSceneObjectByID(vengine::ID id) const;

    void exportScene(const ExportRenderParams &renderParams) const;

    virtual Light *createLight(const AssetInfo &info, LightType type, glm::vec4 color = {1, 1, 1, 1}) = 0;

    virtual InstancesManager &instancesManager() = 0;

protected:
    Engine &m_engine;

    std::shared_ptr<Camera> m_camera = nullptr;
    float m_exposure = 0.0f;

    MaterialSkybox *m_skybox = nullptr;
    EnvironmentType m_environmentType = EnvironmentType::HDRI;
    float m_environmentIntensity = 1.0f;
    glm::vec3 m_backgroundColor = {0.0, 0.0, 0.0};

    std::unordered_map<vengine::ID, SceneObject *> m_objectsMap;

    SceneObjectVector m_sceneGraph;
    bool m_sceneGraphNeedsUpdate = true;
    bool m_instancesNeedUpdate = true;

    virtual SceneObject *createObject(std::string name) = 0;
    virtual void deleteObject(SceneObject *) = 0;
    virtual void invalidateTLAS() = 0;

private:
    void invalidateSceneGraph(bool changed);
    void invalidateInstances(bool changed);
};

}  // namespace vengine

#endif