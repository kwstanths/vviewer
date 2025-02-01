#ifndef __Camera_hpp__
#define __Camera_hpp__

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "math/Transform.hpp"
#include "Material.hpp"

namespace vengine
{

enum class CameraType {
    PERSPECTIVE = 0,
    ORTHOGRAPHIC = 1,
};

class Camera
{
public:
    virtual CameraType type() const = 0;

    Transform &transform();
    const Transform &transform() const;

    Material *&volume();

    const int &width() const;
    const int &height() const;

    const float &aspectRatio() const;

    glm::mat4 viewMatrix() const;
    glm::mat4 viewMatrixInverse() const;

    virtual glm::mat4 projectionMatrix() const = 0;
    virtual glm::mat4 projectionMatrixInverse() const = 0;

    virtual void setWindowSize(int width, int height);

    const float &znear() const;
    float &znear();

    const float &zfar() const;
    float &zfar();

    const float &lensRadius() const;
    float &lensRadius();

    const float &focalDistance() const;
    float &focalDistance();

protected:
    int m_width{};
    int m_height{};
    float m_aspectRatio = 1.F;

private:
    Transform m_transform;
    Material *m_volume = nullptr;

    float m_znear = 0.5F;
    float m_zfar = 50.F;

    float m_lensRadius = 0.0F;
    float m_focalDistance = 10.F;
};

class PerspectiveCamera : public Camera
{
public:
    CameraType type() const override;

    glm::mat4 projectionMatrix() const override;
    glm::mat4 projectionMatrixInverse() const override;

    /* In degrees */
    const float &fov() const;
    float &fov();

private:
    float m_fov = 60.0f;
};

class OrthographicCamera : public Camera
{
public:
    CameraType type() const override;

    glm::mat4 projectionMatrix() const override;
    glm::mat4 projectionMatrixInverse() const override;

    void setOrthoWidth(float orthoWidth);
    const float &orthoWidth() const;
    const float &orthoHeight() const;

protected:
    virtual void setWindowSize(int width, int height) override;

private:
    float m_orthoWidth = 10.F;
    float m_orthoHeight = 10.F;
};

}  // namespace vengine

#endif