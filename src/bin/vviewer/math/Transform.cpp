#include "Transform.hpp"

#include <glm/gtx/quaternion.hpp>
#include <iostream>

namespace vengine {

Transform::Transform()
{
    m_position = glm::vec3(0, 0, 0);
    m_scale = glm::vec3(1, 1, 1);
    m_rotation = glm::quat({ 0, 0, 0 });

    computeBasisVectors();
}

Transform::Transform(glm::vec3 pos)
{
    m_position = pos;
    m_scale = glm::vec3(1, 1, 1);
    m_rotation = glm::quat({ 0, 0, 0 });

    computeBasisVectors();
}

Transform::Transform(glm::vec3 pos, glm::vec3 scale)
{
    m_position = pos;
    m_scale = scale;
    m_rotation = glm::quat({ 0, 0, 0 });
 
    computeBasisVectors();
}

Transform::Transform(glm::vec3 pos, glm::quat rotation)
{
    m_position = pos;
    m_scale = glm::vec3(1, 1, 1);
    m_rotation = rotation;

    computeBasisVectors();
}

Transform::Transform(glm::vec3 pos, glm::vec3 scale, glm::quat rotation)
{
    m_position = pos;
    m_scale = scale;
    m_rotation = rotation;
    
    computeBasisVectors();
}

Transform::Transform(glm::vec3 pos, glm::vec3 scale, glm::vec3 eulerAngles)
{
    m_position = pos;
    m_scale = scale;
    m_rotation = glm::quat(eulerAngles);

    computeBasisVectors();
}

void Transform::setPosition(const glm::vec3 & newPosition)
{
    m_position = newPosition;
}

void Transform::setPosition(float x, float y, float z)
{
    m_position = glm::vec3(x, y, z);
}

glm::vec3 Transform::getPosition() const
{
    return m_position;
}

glm::quat Transform::getRotation() const
{
    return m_rotation;
}

glm::mat4 Transform::getModelMatrix() const
{
    return glm::translate(glm::mat4(1.0f), m_position) * glm::toMat4(m_rotation) * glm::scale(glm::mat4(1.0f), m_scale);
}

void Transform::setRotation(const glm::quat & newRotation)
{
    m_rotation = newRotation;
    computeBasisVectors();
}

void Transform::setRotationEuler(float x, float y, float z)
{
    m_rotation = glm::quat(glm::vec3(x, y, z));
    computeBasisVectors();
}

void Transform::setRotation(glm::vec3 forward, glm::vec3 up)
{
    glm::vec3 newZ = glm::normalize(-forward);
    glm::vec3 newY = glm::normalize(up);

    glm::vec3 newX = glm::normalize(glm::cross(newY, newZ));
    newY = glm::normalize(glm::cross(newZ, newX));
    glm::mat4 rotation = {
        glm::vec4(newX, 0),
        glm::vec4(newY, 0),
        glm::vec4(newZ, 0),
        {0, 0, 0, 1}
    };
    m_rotation = quat_cast(rotation);
    computeBasisVectors();
}

glm::vec3 Transform::getScale() const
{
    return m_scale;
}

void Transform::setScale(const glm::vec3 & newScale)
{
    m_scale = newScale;
}

void Transform::setScale(float x, float y, float z)
{
    m_scale = glm::vec3(x, y, z);
}

void Transform::rotate(const glm::vec3& axis, const float angle)
{
    m_rotation = glm::rotate(m_rotation, angle, axis);
    computeBasisVectors();
}

void Transform::computeBasisVectors()
{
    m_x = glm::rotate(m_rotation, Transform::X);
    m_y = glm::rotate(m_rotation, Transform::Y);
    m_z = glm::rotate(m_rotation, Transform::Z);
}

}