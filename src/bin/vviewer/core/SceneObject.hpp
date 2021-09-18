#ifndef __SceneObject_hpp__
#define __SceneObject_hpp__

#include <math/Transform.hpp>

#include "MeshModel.hpp"

class SceneObject {
public:
    SceneObject() {};

    SceneObject(const MeshModel * meshModel, Transform transform);

    Transform& getTransform();

    const MeshModel * getMeshModel();

protected:
    const MeshModel * m_meshModel = nullptr;
    Transform m_transform;
};

#endif