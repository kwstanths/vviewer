#ifndef __Transform_hpp__
#define __Transform_hpp__

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class Transform {
public:
    Transform();

    void setPosition(glm::vec3& newPosition);
    glm::vec3 getPosition() const;

    glm::quat getRotation() const;
    void setRotation(glm::quat &newRotation);

    glm::vec3 m_forward;
    glm::vec3 m_right;
    glm::vec3 m_up;
private:
    glm::vec3 m_position;
    glm::quat m_rotation;

    void computeBasisVectors();

};

#endif