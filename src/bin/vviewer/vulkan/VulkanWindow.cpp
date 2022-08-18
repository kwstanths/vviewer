#include "VulkanWindow.hpp"

#include <qevent.h>
#include <qapplication.h>

VulkanWindow::VulkanWindow()
{
    m_scene = new VulkanScene();

    /*OrthographicCamera * camera = new OrthographicCamera();
    camera->setOrthoWidth(10.f);*/
    PerspectiveCamera * camera = new PerspectiveCamera();
    camera->setFoV(60.0f);
    
    Transform cameraTransform;
    cameraTransform.setPosition(glm::vec3(1, 3, 10));
    cameraTransform.setRotation(glm::quat(glm::vec3(0, glm::radians(180.0f), 0)));
    camera->getTransform() = cameraTransform;
    
    m_scene->setCamera(std::shared_ptr<Camera>(camera));

    m_updateCameraTimer = new QTimer();
    m_updateCameraTimer->setInterval(16);
    connect(m_updateCameraTimer, SIGNAL(timeout()), this, SLOT(onUpdateCamera()));
    m_updateCameraTimer->start();

    /* Make sure graphics resources are not released and then reinitialized when the window becomes unexposed */
    setFlags(QVulkanWindow::PersistentResources);
}

VulkanWindow::~VulkanWindow()
{

}

QVulkanWindowRenderer * VulkanWindow::createRenderer()
{
    m_renderer = new VulkanRenderer(this, m_scene);

    return m_renderer;
}

void VulkanWindow::resizeEvent(QResizeEvent * ev)
{
    m_scene->getCamera()->setWindowSize(ev->size().width(), ev->size().height());
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
    if (ev->button() == Qt::LeftButton) {
        /* Select object from scene */
        QPointF pos = ev->localPos();
        QSize size = this->size();
        ID objectID = IDGeneration::fromRGB(m_renderer->selectObject(pos.x() / size.width(), pos.y() / size.height()));

        m_selectedPressed = objectID;
    }
}

void VulkanWindow::mouseReleaseEvent(QMouseEvent * ev)
{
    std::shared_ptr<SceneObject> object = m_scene->getSceneObject(m_selectedPressed);
    m_selectedPressed = 0;

    if (object.get() == nullptr) return;

    emit sceneObjectSelected(object);
}

void VulkanWindow::mouseMoveEvent(QMouseEvent * ev)
{
    /* Ignore first movement */
    if (m_mousePosFirst) {
        m_mousePos = ev->localPos();
        m_mousePosFirst = false;
        return;
    }
 
    /* Calculate diff with the previous position */
    QPointF newMousePos = ev->localPos();
    QPointF mousePosDiff = newMousePos - m_mousePos;
    m_mousePos = newMousePos;

    /* Perform camera movement if right button is pressed */
    Qt::MouseButtons buttons = ev->buttons();
    Transform& cameraTransform = m_scene->getCamera()->getTransform();
    if (buttons & Qt::RightButton) {
        float mouseSensitivity = 0.002f;
        
        /* FPS style camera rotation, if middle mouse is pressed while the mouse is dragged over the window */
        glm::quat rotation = cameraTransform.getRotation();
        glm::quat qPitch = glm::angleAxis((float)mousePosDiff.y() * mouseSensitivity, glm::vec3(1, 0, 0));
        glm::quat qYaw = glm::angleAxis(-(float)mousePosDiff.x() * mouseSensitivity, glm::vec3(0, 1, 0));

        cameraTransform.setRotation(glm::normalize(qYaw * rotation * qPitch));
    }

    /* Perform movement pf selected object if left button is pressed */
    float movementSensitivity = 0.02f;
    if (buttons & Qt::LeftButton) {
        std::shared_ptr<SceneObject> selectedObject = m_renderer->getSelectedObject();
        if (selectedObject.get() == nullptr) return;

        Transform& selectedObjectTransform = selectedObject->m_localTransform;
        glm::vec3 position = selectedObjectTransform.getPosition();

        /* The transform UI is rendered with the global right/up/forard vectors, so use these for movement */
        glm::vec3 right = selectedObject->m_modelMatrix * glm::vec4(1, 0, 0, 0);
        glm::vec3 up = selectedObject->m_modelMatrix * glm::vec4(0, 1, 0, 0);
        glm::vec3 forward = selectedObject->m_modelMatrix * glm::vec4(0, 0, 1, 0);
        
        switch (m_selectedPressed)
        {
        case static_cast<ID>(ReservedObjectID::RIGHT_TRANSFORM_ARROW):
        {
            /* Find if right vector is more aligned with the up or right of camera, to use the appropriate mouse diff */
            float cameraRightDot = glm::dot(right, cameraTransform.getRight());
            float cameraUpDot = glm::dot(right, cameraTransform.getUp());
            float movement;
            if (std::abs(cameraRightDot) > std::abs(cameraUpDot)) {
                movement = ((cameraRightDot > 0) ? -1 : 1) * (float)mousePosDiff.x();
            }
            else {
                movement = ((cameraUpDot > 0) ? 1 : -1) * ((float)-mousePosDiff.y());
            }
            position += selectedObjectTransform.getRight() * movementSensitivity * movement;
            
            selectedObjectTransform.setPosition(position);
            emit selectedObjectPositionChanged();
            break;
        }
        case static_cast<ID>(ReservedObjectID::FORWARD_TRANSFORM_ARROW):
        {
            /* Find if forward vector is more aligned with the up or right of camera, to use the appropriate mouse diff */
            float cameraRightDot = glm::dot(forward, cameraTransform.getRight());
            float cameraUpDot = glm::dot(forward, cameraTransform.getUp());
            float movement;
            if (std::abs(cameraRightDot) > std::abs(cameraUpDot)) {
                movement = ((cameraRightDot > 0) ? -1 : 1) * (float)mousePosDiff.x();
            }
            else {
                movement = ((cameraUpDot > 0) ? 1 : -1) * ((float)-mousePosDiff.y());
            }
            position += selectedObjectTransform.getForward() * movementSensitivity * movement;

            selectedObjectTransform.setPosition(position);
            emit selectedObjectPositionChanged();
            break;
        }
        case static_cast<ID>(ReservedObjectID::UP_TRANSFORM_ARROW):
        {
            /* Find if up vector is more aligned with the up or right of camera, to use the appropriate mouse diff */
            float cameraRightDot = glm::dot(up, cameraTransform.getRight());
            float cameraUpDot = glm::dot(up, cameraTransform.getUp());
            float movement;
            if (std::abs(cameraRightDot) > std::abs(cameraUpDot)) {
                movement = ((cameraRightDot > 0) ? -1 : 1) * (float)mousePosDiff.x();
            }
            else {
                movement = ((cameraUpDot > 0) ? 1 : -1) * ((float)-mousePosDiff.y());
            }
            position += selectedObjectTransform.getUp() * movementSensitivity * movement;

            selectedObjectTransform.setPosition(position);
            emit selectedObjectPositionChanged();
            break;
        }
        default:
            break;
        }
    }
}

void VulkanWindow::onUpdateCamera()
{
    /* FPS style camera movement */
    float cameraDefaultSpeed = 0.05f;
    float cameraFastSpeed = 0.3f;

    float speed = cameraDefaultSpeed;
    if (m_keysPressed[Qt::Key_Shift]) speed = cameraFastSpeed;

    Transform& cameraTransform = m_scene->getCamera()->getTransform();
    cameraTransform.setPosition(cameraTransform.getPosition() + static_cast<float>(m_keysPressed[Qt::Key_W]) * cameraTransform.getForward() * speed);
    cameraTransform.setPosition(cameraTransform.getPosition() - static_cast<float>(m_keysPressed[Qt::Key_S]) * cameraTransform.getForward() * speed);
    cameraTransform.setPosition(cameraTransform.getPosition() + static_cast<float>(m_keysPressed[Qt::Key_A]) * cameraTransform.getRight() * speed);
    cameraTransform.setPosition(cameraTransform.getPosition() - static_cast<float>(m_keysPressed[Qt::Key_D]) * cameraTransform.getRight() * speed);
    cameraTransform.setPosition(cameraTransform.getPosition() + static_cast<float>(m_keysPressed[Qt::Key_Q]) * cameraTransform.getUp() * speed);
    cameraTransform.setPosition(cameraTransform.getPosition() - static_cast<float>(m_keysPressed[Qt::Key_E]) * cameraTransform.getUp() * speed);
}
