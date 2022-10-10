#include "Transform.hpp"

#include <glm/gtx/quaternion.hpp>
#include <iostream>

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

glm::vec3 Transform::getForward() const
{
    return m_forward;
}

glm::vec3 Transform::getUp() const
{
    return m_up;
}

glm::vec3 Transform::getRight() const
{
    return m_right;
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

void Transform::setRotation(glm::vec3 direction, glm::vec3 up)
{
    glm::vec3 right = glm::cross(up, direction);
    glm::vec3 newUp = glm::cross(direction, right);
    glm::mat4 rotation = {
        glm::vec4(right, 0),
        glm::vec4(newUp, 0),
        glm::vec4(direction, 0),
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
    m_forward = glm::rotate(m_rotation, getForwardGlobal());
    m_right = glm::rotate(m_rotation, getRightGlobal());
    m_up = glm::rotate(m_rotation, getUpGlobal());
}
