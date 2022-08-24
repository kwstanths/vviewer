#ifndef __Transform_hpp__
#define __Transform_hpp__

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class Transform {
public:
    Transform();
    Transform(glm::vec3 pos);
    Transform(glm::vec3 pos, glm::vec3 scale);
    Transform(glm::vec3 pos, glm::quat rotation);
    Transform(glm::vec3 pos, glm::vec3 scale, glm::quat rotation);
    Transform(glm::vec3 pos, glm::vec3 scale, glm::vec3 eulerAngles);

    void setPosition(const glm::vec3& newPosition);
    void setPosition(float x, float y, float z);
    glm::vec3 getPosition() const;

    void setRotation(const glm::quat &newRotation);
    void setRotationEuler(float x, float y, float z);
    void setRotation(glm::vec3 direction, glm::vec3 up);
    glm::quat getRotation() const;

    void setScale(const glm::vec3 &newScale);
    void setScale(float x, float y, float z);
    glm::vec3 getScale() const;

    glm::vec3 getForward() const;
    glm::vec3 getUp() const;
    glm::vec3 getRight() const;

    glm::mat4 getModelMatrix() const;

    void rotate(const glm::vec3& axis, const float angle);

private:
    glm::vec3 m_position;
    glm::quat m_rotation;
    glm::vec3 m_scale;

    glm::vec3 m_forward;
    glm::vec3 m_right;
    glm::vec3 m_up;

    void computeBasisVectors();

};

#endif