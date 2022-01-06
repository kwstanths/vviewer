#ifndef __Lights_hpp__
#define __Lights_hpp__

#include "glm/glm.hpp"

#include "math/Transform.hpp"

struct Light {
    glm::vec3 color;
    Transform transform;

    Light(Transform t, glm::vec3 c) : transform(t), color(c) {};
};

struct DirectionalLight : public Light {
    DirectionalLight(Transform t, glm::vec3 c) : Light(t, c) {};
};

#endif