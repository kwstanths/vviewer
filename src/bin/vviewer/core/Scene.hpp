#ifndef __Scene_hpp__
#define __Scene_hpp__

#include <memory>

#include "Camera.hpp"
#include "Lights.hpp"
#include "SceneObject.hpp"

class Scene {
public:
    Scene();
    ~Scene();

    void setCamera(std::shared_ptr<Camera> camera);
    std::shared_ptr<Camera> getCamera() const;

    void setDirectionalLight(std::shared_ptr<DirectionalLight> directionaLight);
    std::shared_ptr<DirectionalLight> getDirectionalLight() const;

    virtual SceneObject* addSceneObject(std::string meshModel, Transform transform, std::string material) = 0;

protected:
    std::shared_ptr<Camera> m_camera;
    std::shared_ptr<DirectionalLight> m_directionalLight;
};

#endif