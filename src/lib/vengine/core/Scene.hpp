#ifndef __Scene_hpp__
#define __Scene_hpp__

#include <memory>
#include <utility>

#include <vengine/utils/IDGeneration.hpp>

#include "Camera.hpp"
#include "Lights.hpp"
#include "SceneObject.hpp"
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

struct SceneData {
    glm::mat4 m_view;
    glm::mat4 m_viewInverse;
    glm::mat4 m_projection;
    glm::mat4 m_projectionInverse;
    glm::vec4 m_exposure; /* R = exposure, G = ambient environment map multiplier, B = , A = */
};

class Scene
{
public:
    Scene();
    ~Scene();

    const std::shared_ptr<Camera> &camera() const { return m_camera; }
    std::shared_ptr<Camera> &camera() { return m_camera; }

    const float &exposure() const { return m_exposure; }
    float &exposure() { return m_exposure; }

    const float &ambientIBLFactor() const { return m_ambientIBL; }
    float &ambientIBLFactor() { return m_ambientIBL; }

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

    void updateSceneGraph();

    SceneGraph &sceneGraph();
    /* Get all scene objects in a flat array */
    SceneGraph getSceneObjectsArray() const;
    SceneGraph getSceneObjectsArray(std::vector<glm::mat4> &modelMatrices) const;

    SceneObject *getSceneObject(vengine::ID id) const;

    void exportScene(const ExportRenderParams &renderParams) const;

protected:
    std::shared_ptr<Camera> m_camera = nullptr;
    float m_exposure = 0.0f;
    float m_ambientIBL = 1.0f;

    MaterialSkybox *m_skybox;
    EnvironmentType m_environmentType = EnvironmentType::HDRI;
    glm::vec3 m_backgroundColor = {0, 0.5, 0.5};

    std::unordered_map<vengine::ID, SceneObject *> m_objectsMap;

    virtual SceneObject *createObject(std::string name) = 0;
    virtual void deleteObject(SceneObject *) = 0;

    SceneGraph m_sceneGraph;
};

}  // namespace vengine

#endif