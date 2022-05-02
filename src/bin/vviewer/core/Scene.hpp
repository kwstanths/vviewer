#ifndef __Scene_hpp__
#define __Scene_hpp__

#include <memory>

#include "Camera.hpp"
#include "Lights.hpp"
#include "SceneObject.hpp"
#include "SceneGraph.hpp"

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

    /* Add a new scene object at the root of the scene graph */
    virtual std::shared_ptr<SceneNode> addSceneObject(std::string meshModel, Transform transform, std::string material) = 0;
    /* Add a new scene object as a child of a node */
    virtual std::shared_ptr<SceneNode> addSceneObject(std::shared_ptr<SceneNode> node, std::string meshModel, Transform transform, std::string material) = 0;

    void removeSceneObject(std::shared_ptr<SceneNode> node);

    void updateSceneGraph();
    std::vector<std::shared_ptr<SceneObject>> getSceneObjects();

protected:
    float m_exposure = 0.0f;
    std::shared_ptr<Camera> m_camera;
    std::shared_ptr<DirectionalLight> m_directionalLight;
    std::vector<std::shared_ptr<SceneNode>> m_sceneGraph;
};

#endif