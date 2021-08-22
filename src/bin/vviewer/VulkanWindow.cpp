#include "VulkanWindow.hpp"

#include <qevent.h>
#include <qapplication.h>

#include "math/Transform.hpp"

VulkanWindow::VulkanWindow()
{
    /*OrthographicCamera * camera = new OrthographicCamera();
    camera->setOrthoWidth(10.f);*/
    PerspectiveCamera * camera = new PerspectiveCamera();
    camera->setFoV(45.0f);
    
    Transform cameraTransform;
    cameraTransform.setPosition(glm::vec3(5, 0, 0));
    cameraTransform.setRotation(glm::quat(glm::vec3(0, glm::radians(-90.0f), 0)));
    
    m_camera = std::shared_ptr<Camera>(camera);
    m_camera->getTransform() = cameraTransform;

    m_updateCameraTimer = new QTimer();
    m_updateCameraTimer->setInterval(16);
    connect(m_updateCameraTimer, SIGNAL(timeout()), this, SLOT(onUpdateCamera()));
    m_updateCameraTimer->start();
}

QVulkanWindowRenderer * VulkanWindow::createRenderer()
{
    VulkanRenderer * renderer = new VulkanRenderer(this);

    renderer->setCamera(m_camera);

    return renderer;
}

void VulkanWindow::resizeEvent(QResizeEvent * ev)
{
    m_camera->setWindowSize(ev->size().width(), ev->size().height());
}

void VulkanWindow::keyPressEvent(QKeyEvent * ev)
{
    if (ev->key() == Qt::Key_W) {
        m_keysPressed[Qt::Key_W] = true;
    }
    if (ev->key() == Qt::Key_A) {
        m_keysPressed[Qt::Key_A] = true;
    }
    if (ev->key() == Qt::Key_S) {
        m_keysPressed[Qt::Key_S] = true;
    }
    if (ev->key() == Qt::Key_D) {
        m_keysPressed[Qt::Key_D] = true;
    }

}

void VulkanWindow::keyReleaseEvent(QKeyEvent * ev)
{
    if (ev->key() == Qt::Key_W) {
        m_keysPressed[Qt::Key_W] = false;
    }
    if (ev->key() == Qt::Key_A) {
        m_keysPressed[Qt::Key_A] = false;
    }
    if (ev->key() == Qt::Key_S) {
        m_keysPressed[Qt::Key_S] = false;
    }
    if (ev->key() == Qt::Key_D) {
        m_keysPressed[Qt::Key_D] = false;
    }
}

void VulkanWindow::mousePressEvent(QMouseEvent * ev)
{
}

void VulkanWindow::mouseReleaseEvent(QMouseEvent * ev)
{
}

void VulkanWindow::mouseMoveEvent(QMouseEvent * ev)
{
    /* Ignore first movement */
    if (m_mousePosFirst) {
        m_mousePos = ev->localPos();
        m_mousePosFirst = false;
        return;
    }
 
    QPointF newMousePos = ev->localPos();
    QPointF mousePosDiff = m_mousePos - newMousePos;
    m_mousePos = newMousePos;

    float mouseSensitivity = 0.004f;
    Qt::MouseButtons buttons = ev->buttons();
    Transform& cameraTransform = m_camera->getTransform();
    if (buttons & Qt::MiddleButton) {
        /* FPS style camera rotation, if middle mouse is pressed while the mouse is dragged over the window */
        glm::quat rotation = cameraTransform.getRotation();
        glm::quat qPitch = glm::angleAxis((float)-mousePosDiff.y() * mouseSensitivity, glm::vec3(1, 0, 0));
        glm::quat qYaw = glm::angleAxis((float)-mousePosDiff.x() * mouseSensitivity, glm::vec3(0, 1, 0));

        cameraTransform.setRotation(glm::normalize(qPitch * rotation * qYaw));
    }
}

void VulkanWindow::onUpdateCamera()
{
    /* FPS style camera movement */
    float cameraSpeed = 0.1f;
    Transform& cameraTransform = m_camera->getTransform();
    cameraTransform.setPosition(cameraTransform.getPosition() + static_cast<float>(m_keysPressed[Qt::Key_W]) * cameraTransform.getForward() * cameraSpeed);
    cameraTransform.setPosition(cameraTransform.getPosition() - static_cast<float>(m_keysPressed[Qt::Key_A]) * cameraTransform.getRight() * cameraSpeed);
    cameraTransform.setPosition(cameraTransform.getPosition() - static_cast<float>(m_keysPressed[Qt::Key_S]) * cameraTransform.getForward() * cameraSpeed);
    cameraTransform.setPosition(cameraTransform.getPosition() + static_cast<float>(m_keysPressed[Qt::Key_D]) * cameraTransform.getRight() * cameraSpeed);
}
