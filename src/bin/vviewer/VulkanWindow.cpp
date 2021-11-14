#include "VulkanWindow.hpp"

#include <qevent.h>
#include <qapplication.h>

VulkanWindow::VulkanWindow()
{
    /*OrthographicCamera * camera = new OrthographicCamera();
    camera->setOrthoWidth(10.f);*/
    PerspectiveCamera * camera = new PerspectiveCamera();
    camera->setFoV(60.0f);
    
    Transform cameraTransform;
    cameraTransform.setPosition(glm::vec3(10, 3, 0));
    cameraTransform.setRotation(glm::quat(glm::vec3(0, glm::radians(-90.0f), 0)));
    
    m_camera = std::shared_ptr<Camera>(camera);
    m_camera->getTransform() = cameraTransform;

    m_updateCameraTimer = new QTimer();
    m_updateCameraTimer->setInterval(16);
    connect(m_updateCameraTimer, SIGNAL(timeout()), this, SLOT(onUpdateCamera()));
    m_updateCameraTimer->start();
}

VulkanWindow::~VulkanWindow()
{

}

QVulkanWindowRenderer * VulkanWindow::createRenderer()
{
    m_renderer = new VulkanRenderer(this);

    m_renderer->setCamera(m_camera);

    return m_renderer;
}

void VulkanWindow::resizeEvent(QResizeEvent * ev)
{
    m_camera->setWindowSize(ev->size().width(), ev->size().height());
}

void VulkanWindow::keyPressEvent(QKeyEvent * ev)
{
    m_keysPressed[ev->key()] = true;
}

void VulkanWindow::keyReleaseEvent(QKeyEvent * ev)
{
    m_keysPressed[ev->key()] = false;
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

    float mouseSensitivity = 0.002f;
    Qt::MouseButtons buttons = ev->buttons();
    Transform& cameraTransform = m_camera->getTransform();
    if (buttons & Qt::RightButton) {
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
    cameraTransform.setPosition(cameraTransform.getPosition() + static_cast<float>(m_keysPressed[Qt::Key_Q]) * cameraTransform.getUp() * cameraSpeed);
    cameraTransform.setPosition(cameraTransform.getPosition() - static_cast<float>(m_keysPressed[Qt::Key_E]) * cameraTransform.getUp() * cameraSpeed);
}
