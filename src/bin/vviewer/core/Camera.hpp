#ifndef __Camera_hpp__
#define __Camera_hpp__

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "math/Transform.hpp"

enum class CameraType {
    PERSPECTIVE = 0,
    ORTHOGRAPHIC = 1,
};

class Camera {
public:

    virtual CameraType getType() const = 0;

    glm::mat4 getViewMatrix() const;
    glm::mat4 getViewMatrixInverse() const;
    virtual glm::mat4 getProjectionMatrix() const = 0;
    virtual glm::mat4 getProjectionMatrixInverse() const = 0;

    virtual void setWindowSize(int width, int height);

    int getWidth() const;
    int getHeight() const;
    float getAspectRatio() const;

    Transform& getTransform();

protected:
    int m_width{};
    int m_height{};
    float m_aspectRatio = 1.F;

private:
    Transform m_transform;
};

class PerspectiveCamera : public Camera {
public:
    CameraType getType() const;

    glm::mat4 getProjectionMatrix() const;
    glm::mat4 getProjectionMatrixInverse() const;

    /* In degrees */
    void setFoV(float fov);
    float getFoV() const;
private:
    float m_fov = 60.0f;
};

class OrthographicCamera : public Camera {
public:
    CameraType getType() const;

    glm::mat4 getProjectionMatrix() const;
    glm::mat4 getProjectionMatrixInverse() const;

    void setOrthoWidth(float orthoWidth);

protected:
    virtual void setWindowSize(int width, int height) override;

private:
    float m_orthoWidth = 10.F;
    float m_orthoHeight = 10.F;

};

#endif