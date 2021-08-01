#include "Camera.hpp"

glm::mat4 Camera::getViewMatrix() const
{
    return glm::lookAt(glm::vec3(5.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

void Camera::setWindowSize(int width, int height)
{
    m_width = width;
    m_height = height;
    m_aspectRatio = (float)m_width / (float)m_height;
}



glm::mat4 PerspectiveCamera::getProjectionMatrix() const
{
    return glm::perspective(glm::radians(45.0f), m_aspectRatio, 0.1f, 100.0f);
}

glm::mat4 OrthographicCamera::getProjectionMatrix() const
{
    return glm::ortho((float)-m_orthoWidth /2, (float)m_orthoWidth /2, (float)-m_orthoHeight /2, (float)m_orthoHeight / 2, -10.0f, 10.0f);
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
