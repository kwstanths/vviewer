#include "Camera.hpp"

#include <glm/gtx/quaternion.hpp>

glm::mat4 Camera::getViewMatrix() const
{
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), -m_transform.getPosition());
    glm::mat4 rotation = glm::toMat4(m_transform.getRotation());
    glm::mat4 view = rotation * translation;

    return view;
}

void Camera::setWindowSize(int width, int height)
{
    m_width = width;
    m_height = height;
    m_aspectRatio = (float)m_width / (float)m_height;
}

float Camera::getAspectRatio() const
{
    return m_aspectRatio;
}

Transform & Camera::getTransform()
{
    return m_transform;
}

int Camera::getWidth() const
{
    return m_width;
}

int Camera::getHeight() const
{
    return m_height;
}



glm::mat4 PerspectiveCamera::getProjectionMatrix() const
{
    return glm::perspective(glm::radians(m_fov), m_aspectRatio, 0.01f, 50.0f);
}

void PerspectiveCamera::setFoV(float fov)
{
    m_fov = fov;
}



glm::mat4 OrthographicCamera::getProjectionMatrix() const
{
    return glm::ortho((float)-m_orthoWidth /2, (float)m_orthoWidth /2, (float)-m_orthoHeight /2, (float)m_orthoHeight / 2, -100.0f, 100.0f);
}

void OrthographicCamera::setOrthoWidth(float orthoWidth)
{
    m_orthoWidth = orthoWidth;
    m_orthoHeight = m_orthoWidth / m_aspectRatio;
}

void OrthographicCamera::setWindowSize(int width, int height)
{
    Camera::setWindowSize(width, height);
    m_orthoHeight = m_orthoWidth / m_aspectRatio;
}