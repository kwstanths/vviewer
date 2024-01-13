#include "Camera.hpp"

#include <glm/gtx/quaternion.hpp>

namespace vengine
{

const float &Camera::aspectRatio() const
{
    return m_aspectRatio;
}

const Transform &Camera::transform() const
{
    return m_transform;
}

Transform &Camera::transform()
{
    return m_transform;
}

const int &Camera::width() const
{
    return m_width;
}

const int &Camera::height() const
{
    return m_height;
}

glm::mat4 Camera::viewMatrix() const
{
    return glm::lookAt(m_transform.position(), m_transform.position() + m_transform.forward(), m_transform.up());
}

glm::mat4 Camera::viewMatrixInverse() const
{
    /* TODO optimise this */
    return glm::inverse(viewMatrix());
}

void Camera::setWindowSize(int width, int height)
{
    m_width = width;
    m_height = height;
    m_aspectRatio = (float)m_width / (float)m_height;
}

const float &Camera::znear() const
{
    return m_znear;
}
float &Camera::znear()
{
    return m_znear;
}

const float &Camera::zfar() const
{
    return m_zfar;
}

float &Camera::zfar()
{
    return m_zfar;
}

const float &Camera::lensRadius() const
{
    return m_lensRadius;
}

float &Camera::lensRadius()
{
    return m_lensRadius;
}

const float &Camera::focalDistance() const
{
    return m_focalDistance;
}

float &Camera::focalDistance()
{
    return m_focalDistance;
}

/* ------------- PerspectiveCamera ------------- */

CameraType PerspectiveCamera::type() const
{
    return CameraType::PERSPECTIVE;
}

glm::mat4 PerspectiveCamera::projectionMatrix() const
{
    assert(m_aspectRatio != 0);
    return glm::perspective(glm::radians(m_fov), m_aspectRatio, znear(), zfar());
}

glm::mat4 PerspectiveCamera::projectionMatrixInverse() const
{
    /* TODO optimize this */
    return glm::inverse(projectionMatrix());
}

const float &PerspectiveCamera::fov() const
{
    return m_fov;
}

float &PerspectiveCamera::fov()
{
    return m_fov;
}

CameraType OrthographicCamera::type() const
{
    return CameraType::ORTHOGRAPHIC;
}

glm::mat4 OrthographicCamera::projectionMatrix() const
{
    return glm::ortho(-m_orthoWidth / 2, m_orthoWidth / 2, -m_orthoHeight / 2, m_orthoHeight / 2, znear(), zfar());
}

glm::mat4 OrthographicCamera::projectionMatrixInverse() const
{
    /* TODO optimize this */
    return glm::inverse(projectionMatrix());
}

void OrthographicCamera::setOrthoWidth(float orthoWidth)
{
    m_orthoWidth = orthoWidth;

    assert(m_aspectRatio != 0);
    m_orthoHeight = m_orthoWidth / m_aspectRatio;
}

const float &OrthographicCamera::orthoWidth() const
{
    return m_orthoWidth;
}

const float &OrthographicCamera::orthoHeight() const
{
    return m_orthoHeight;
}

void OrthographicCamera::setWindowSize(int width, int height)
{
    Camera::setWindowSize(width, height);

    assert(m_aspectRatio != 0);
    m_orthoHeight = m_orthoWidth / m_aspectRatio;
}

}  // namespace vengine