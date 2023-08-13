#ifndef __Renderer_hpp__
#define __Renderer_hpp__

#include "SceneObject.hpp"

class Renderer {
public:

    Renderer();

    void setSelectedObject(std::shared_ptr<SceneObject> sceneObject) { m_selectedObject = sceneObject; }
    std::shared_ptr<SceneObject> getSelectedObject() const { return m_selectedObject; }

protected:
    std::shared_ptr<SceneObject> m_selectedObject;

private:

};

#endif