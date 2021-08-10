#ifndef __Transform_hpp__
#define __Transform_hpp__

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class Transform {
public:
    Transform();

    void setPosition(const glm::vec3& newPosition);
    glm::vec3 getPosition() const;

    glm::quat getRotation() const;
    void setRotation(const glm::quat &newRotation);

    glm::vec3 getForward() const;
    glm::vec3 getUp() const;
    glm::vec3 getRight() const;

    void rotate(const glm::vec3& axis, const float angle);

private:
    glm::vec3 m_position;
    glm::quat m_rotation;

    glm::vec3 m_forward;
    glm::vec3 m_right;
    glm::vec3 m_up;

    void computeBasisVectors();

};

#endif