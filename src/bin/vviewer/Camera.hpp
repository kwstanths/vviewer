#ifndef __Camera_hpp__
#define __Camera_hpp__

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:

    glm::mat4 getViewMatrix() const;
    virtual glm::mat4 getProjectionMatrix() const = 0;

    virtual void setWindowSize(int width, int height);

protected:
    int m_width;
    int m_height;
    float m_aspectRatio;

private:

};

class PerspectiveCamera : public Camera {
public:

    glm::mat4 getProjectionMatrix() const;

private:

};

class OrthographicCamera : public Camera {
public:

    glm::mat4 getProjectionMatrix() const;

    void setOrthoWidth(float orthoWidth);

protected:
    virtual void setWindowSize(int width, int height) override;

private:
    float m_orthoWidth;
    float m_orthoHeight;

};

#endif