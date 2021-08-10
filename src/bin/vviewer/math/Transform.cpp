#include "Transform.hpp"

#include <glm/gtx/quaternion.hpp>
#include <iostream>

Transform::Transform()
{
    m_position = glm::vec3(0, 0, 0);
    m_rotation = glm::quat();

    computeBasisVectors();
}

void Transform::setPosition(const glm::vec3 & newPosition)
{
    m_position = newPosition;
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

void Transform::setRotation(const glm::quat & newRotation)
{
    m_rotation = newRotation;
    computeBasisVectors();
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
