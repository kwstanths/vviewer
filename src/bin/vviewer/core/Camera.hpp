#ifndef __Camera_hpp__
#define __Camera_hpp__

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "math/Transform.hpp"

class Camera {
public:

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
    int m_width;
    int m_height;
    float m_aspectRatio;

private:
    Transform m_transform;
};

class PerspectiveCamera : public Camera {
public:

    glm::mat4 getProjectionMatrix() const;
    glm::mat4 getProjectionMatrixInverse() const;

    void setFoV(float fov);

private:
    float m_fov;
};

class OrthographicCamera : public Camera {
public:

    glm::mat4 getProjectionMatrix() const;
    glm::mat4 getProjectionMatrixInverse() const;

    void setOrthoWidth(float orthoWidth);

protected:
    virtual void setWindowSize(int width, int height) override;

private:
    float m_orthoWidth;
    float m_orthoHeight;

};

#endif