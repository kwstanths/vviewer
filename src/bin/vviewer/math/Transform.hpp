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
    glm::vec3 getForward() const;
    glm::vec3 getUp() const;
    glm::vec3 getRight() const;
    void setRotation(glm::quat &newRotation);

    void rotate(glm::vec3 axis, float angle);

    static glm::vec3 getGlobalX() { return glm::vec3(1, 0, 0); };
    static glm::vec3 getGlobalY() { return glm::vec3(0, 1, 0); };
    static glm::vec3 getGlobalZ() { return glm::vec3(0, 0, 1); };

private:
    glm::vec3 m_position;
    glm::quat m_rotation;

    glm::vec3 m_forward;
    glm::vec3 m_right;
    glm::vec3 m_up;

    void computeBasisVectors();

};

#endif