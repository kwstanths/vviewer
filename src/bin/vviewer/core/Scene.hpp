#ifndef __Scene_hpp__
#define __Scene_hpp__

#include <memory>
#include <utility>

#include "Camera.hpp"
#include "Lights.hpp"
#include "SceneObject.hpp"
#include "IDGeneration.hpp"

struct SceneData {
    glm::mat4 m_view;
    glm::mat4 m_viewInverse;
    glm::mat4 m_projection;
    glm::mat4 m_projectionInverse;
    glm::vec4 m_directionalLightDir;
    glm::vec4 m_directionalLightColor;
    glm::vec3 m_exposure; /* R = exposure, G = , B = , A = */
};

class Scene {
public:
    Scene();
    ~Scene();

    void setCamera(std::shared_ptr<Camera> camera);
    std::shared_ptr<Camera> getCamera() const;

    void setDirectionalLight(std::shared_ptr<DirectionalLight> directionaLight);
    std::shared_ptr<DirectionalLight> getDirectionalLight() const;

    float getExposure() const;
    void setExposure(float exposure);

    virtual SceneData getSceneData() const;

    void setSkybox(MaterialSkybox* skybox);
    MaterialSkybox* getSkybox() const;

    /* Add a new scene object at the root of the scene graph */
    std::shared_ptr<SceneObject> addSceneObject(std::string meshModel, Transform transform, std::string material);
    /* Add a new scene object as a child of a node */
    std::shared_ptr<SceneObject> addSceneObject(std::shared_ptr<SceneObject> node, std::string meshModel, Transform transform, std::string material);

    void removeSceneObject(std::shared_ptr<SceneObject> node);

    void updateSceneGraph();
    std::vector<std::shared_ptr<SceneObject>> getSceneObjects() const;
    std::vector<std::shared_ptr<SceneObject>> getSceneObjects(std::vector<glm::mat4>& modelMatrices) const;

    std::shared_ptr<SceneObject> getSceneObject(ID id) const;
    
    void exportScene(std::string name, uint32_t width, uint32_t height, uint32_t samples) const;

protected:
    float m_exposure = 0.0f;
    
    std::shared_ptr<Camera> m_camera;
    std::shared_ptr<DirectionalLight> m_directionalLight;
    
    std::vector<std::shared_ptr<SceneObject>> m_sceneGraph;
    std::unordered_map<ID, std::shared_ptr<SceneObject>> m_objectsMap;

    MaterialSkybox* m_skybox = nullptr;

    virtual std::vector<std::shared_ptr<SceneObject>> createObject(std::string meshModel, std::string material) = 0;
};

#endif