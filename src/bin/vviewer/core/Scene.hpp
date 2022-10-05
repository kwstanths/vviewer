#ifndef __Scene_hpp__
#define __Scene_hpp__

#include <memory>
#include <utility>

#include <utils/IDGeneration.hpp>
#include "Camera.hpp"
#include "Lights.hpp"
#include "SceneObject.hpp"

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
    glm::vec4 m_directionalLightDir;
    glm::vec4 m_directionalLightColor;
    glm::vec3 m_exposure; /* R = exposure, G = Ambient IBL multiplier, B = , A = */
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

    /* A multiplier of the IBL contribution */
    float getAmbientIBL() const;
    void setAmbientIBL(float ambientIBL);

    void setSkybox(MaterialSkybox* skybox);
    MaterialSkybox* getSkybox() const;

    EnvironmentType getEnvironmentType() const;
    void setEnvironmentType(EnvironmentType type);

    glm::vec3 getBackgroundColor() const;
    void setBackgroundColor(glm::vec3 color);
    
    virtual SceneData getSceneData() const;

    /* Add a new scene object at the root of the scene graph */
    std::shared_ptr<SceneObject> addSceneObject(std::string name, Transform transform);
    /* Add a new scene object as a child of a node */
    std::shared_ptr<SceneObject> addSceneObject(std::string name, std::shared_ptr<SceneObject> node, Transform transform);

    void removeSceneObject(std::shared_ptr<SceneObject> node);

    void updateSceneGraph();
    std::vector<std::shared_ptr<SceneObject>> getSceneObjects() const;
    std::vector<std::shared_ptr<SceneObject>> getSceneObjects(std::vector<glm::mat4>& modelMatrices) const;

    std::shared_ptr<SceneObject> getSceneObject(ID id) const;
    
    void exportScene(std::string name, uint32_t width, uint32_t height, uint32_t samples) const;

protected:
    float m_exposure = 0.0f;
    float m_ambientIBL = 1.0f;
    
    std::shared_ptr<Camera> m_camera;
    std::shared_ptr<DirectionalLight> m_directionalLight;
    
    std::vector<std::shared_ptr<SceneObject>> m_sceneGraph;
    std::unordered_map<ID, std::shared_ptr<SceneObject>> m_objectsMap;

    EnvironmentType m_environmentType = EnvironmentType::HDRI;
    MaterialSkybox* m_skybox = nullptr;
    glm::vec3 m_backgroundColor = { 0, 0.5, 0.5 };

    virtual std::shared_ptr<SceneObject> createObject(std::string name) = 0;
    
    void removeIDs(std::vector<std::shared_ptr<SceneObject>>& objects);
};

#endif