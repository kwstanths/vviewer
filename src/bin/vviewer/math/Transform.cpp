#include "Transform.hpp"

#include <glm/gtx/quaternion.hpp>
#include <iostream>

Transform::Transform()
{
    m_position = glm::vec3(0, 0, 0);
    m_scale = glm::vec3(1, 1, 1);
    m_rotation = glm::quat();

    computeBasisVectors();
}

Transform::Transform(glm::vec3 pos)
{
    m_position = pos;
    m_scale = glm::vec3(1, 1, 1);
    m_rotation = glm::quat();
}

Transform::Transform(glm::vec3 pos, glm::vec3 scale)
{
    m_position = pos;
    m_scale = scale;
    m_rotation = glm::quat();
}

Transform::Transform(glm::vec3 pos, glm::quat rotation)
{
    m_position = pos;
    m_scale = glm::vec3(1, 1, 1);
    m_rotation = rotation;
}

Transform::Transform(glm::vec3 pos, glm::vec3 scale, glm::quat rotation)
{
    m_position = pos;
    m_scale = scale;
    m_rotation = rotation;
}

Transform::Transform(glm::vec3 pos, glm::vec3 scale, glm::vec3 eulerAngles)
{
    m_position = pos;
    m_scale = scale;
    m_rotation = glm::quat(eulerAngles);
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
    glm::quat invQuat = glm::inverse(m_rotation);
    m_forward = glm::rotate(invQuat, glm::vec3(0, 0, -1));
    m_right = glm::rotate(invQuat, glm::vec3(1, 0, 0));
    m_up = glm::rotate(invQuat, glm::vec3(0, 1, 0));
}
