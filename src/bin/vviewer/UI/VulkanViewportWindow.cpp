#include "VulkanViewportWindow.hpp"

#include <QPlatformSurfaceEvent>

#include "vulkan/common/VulkanLimits.hpp"

using namespace vengine;

VulkanViewportWindow::VulkanViewportWindow()
    : ViewportWindow()
{
    setSurfaceType(QSurface::SurfaceType::VulkanSurface);

    m_engine = new VulkanEngine("vviewer");

    m_vkinstance = new QVulkanInstance();
    m_vkinstance->setVkInstance(m_engine->context().instance());
    m_vkinstance->create();
    setVulkanInstance(m_vkinstance);

    /* Create default camera */
    {
        auto camera = std::make_shared<PerspectiveCamera>();
        camera->fov() = 60.0f;
        camera->transform().position() = glm::vec3(0, 3, 10);
        camera->transform().setRotation(glm::quat(glm::vec3(glm::radians(-15.F), 0, 0)));

        m_engine->scene().camera() = camera;
    }

    m_updateCameraTimer = new QTimer();
    m_updateCameraTimer->setInterval(16);
    connect(m_updateCameraTimer, SIGNAL(timeout()), this, SLOT(onUpdateCamera()));
    m_updateCameraTimer->start();
}

VulkanViewportWindow::~VulkanViewportWindow()
{
}

Engine *VulkanViewportWindow::engine() const
{
    return m_engine;
}

void VulkanViewportWindow::windowActivated(bool activated)
{
    if (activated && m_initialized)
        m_engine->start();
    else
        m_engine->stop();
}

void VulkanViewportWindow::releaseResources()
{
    m_engine->releaseSwapChainResources();
    m_engine->releaseResources();
}

void VulkanViewportWindow::exposeEvent(QExposeEvent *event)
{
    if (isExposed() && !m_initialized) {
        /* Perform first time initialization the first time the surface window becomes available */
        VkSurfaceKHR surface = m_vkinstance->surfaceForWindow(this);

        m_engine->initResources(surface);
        m_engine->initSwapChainResources(static_cast<uint32_t>(size().width()), static_cast<uint32_t>(size().height()));

        Q_EMIT initializationFinished();

        m_initialized = true;

        m_engine->start();
    }
}

bool VulkanViewportWindow::event(QEvent *event)
{
    switch (event->type()) {
        case QEvent::PlatformSurface: {
            if (static_cast<QPlatformSurfaceEvent *>(event)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
                m_engine->exit();
                m_engine->waitIdle();

                releaseResources();
            }
            break;
        }
        default: {
        }
    }

    return QWindow::event(event);
}

void VulkanViewportWindow::resizeEvent(QResizeEvent *ev)
{
    m_engine->scene().camera()->setWindowSize(ev->size().width(), ev->size().height());

    VulkanSwapchain &swapchain = m_engine->swapchain();

    /* If the swapchain has been initialized, then we have to recreate it for the new size */
    if (swapchain.isInitialized()) {
        /* Stop the render loop and wait for idle */
        m_engine->stop();
        m_engine->waitIdle();

        /* Reinitialize swapchain based resources */
        m_engine->releaseSwapChainResources();
        m_engine->initSwapChainResources(static_cast<uint32_t>(ev->size().width()), static_cast<uint32_t>(ev->size().height()));

        /* continue the render loop */
        m_engine->start();
    }
}

void VulkanViewportWindow::keyPressEvent(QKeyEvent *ev)
{
    m_keysPressed[ev->key()] = true;
}

void VulkanViewportWindow::keyReleaseEvent(QKeyEvent *ev)
{
    m_keysPressed[ev->key()] = false;
}

void VulkanViewportWindow::mousePressEvent(QMouseEvent *ev)
{
    if (ev->button() == Qt::LeftButton) {
        /* Select object from scene */
        QPointF pos = ev->position();
        QSize size = this->size();

        VulkanRenderer &renderer = static_cast<VulkanRenderer &>(m_engine->renderer());
        ID objectID = renderer.findID(pos.x() / size.width(), pos.y() / size.height());

        m_selectedPressed = objectID;
    }
}

void VulkanViewportWindow::mouseReleaseEvent(QMouseEvent *ev)
{
    SceneObject *object = m_engine->scene().findSceneObjectByID(m_selectedPressed);
    m_selectedPressed = 0;

    if (object == nullptr)
        return;

    Q_EMIT sceneObjectSelected(object);
}

void VulkanViewportWindow::mouseMoveEvent(QMouseEvent *ev)
{
    /* Ignore first movement */
    if (m_mousePosFirst) {
        m_mousePos = ev->position();
        m_mousePosFirst = false;
        return;
    }

    /* Calculate diff with the previous position */
    QPointF newMousePos = ev->position();
    QPointF mousePosDiff = newMousePos - m_mousePos;
    m_mousePos = newMousePos;

    float delta = m_engine->delta();

    /* Perform camera movement if right button is pressed */
    Qt::MouseButtons buttons = ev->buttons();
    Transform &cameraTransform = m_engine->scene().camera()->transform();
    if (buttons & Qt::RightButton) {
        float speed = m_rotateSensitivity * delta;

        /* FPS style camera rotation, if middle mouse is pressed while the mouse is dragged over the window */
        glm::quat rotation = cameraTransform.rotation();
        glm::quat qPitch = glm::angleAxis(static_cast<float>(mousePosDiff.y()) * speed, glm::vec3(1, 0, 0));
        glm::quat qYaw = glm::angleAxis(static_cast<float>(mousePosDiff.x()) * speed, glm::vec3(0, 1, 0));

        cameraTransform.setRotation(glm::normalize(qYaw * rotation * qPitch));
    }

    if (buttons & Qt::MiddleButton) {
        float speed = m_panSensitivity * delta;
        cameraTransform.position() += cameraTransform.right() * static_cast<float>(-mousePosDiff.x()) * speed;
        cameraTransform.position() += cameraTransform.up() * static_cast<float>(mousePosDiff.y()) * speed;
    }

    /* Perform movement of selected object if left button is pressed */
    VulkanRenderer &renderer = static_cast<VulkanRenderer &>(m_engine->renderer());
    if (buttons & Qt::LeftButton) {
        float speed = m_movementSensitivity * delta;
        SceneObject *selectedObject = renderer.getSelectedObject();
        if (selectedObject == nullptr)
            return;

        auto selectedObjectTransform = selectedObject->localTransform();
        glm::vec3 position = selectedObjectTransform.position();

        /* Get the basis vectors of the selected object */
        glm::vec3 objectX = selectedObject->modelMatrix() * glm::vec4(Transform::WORLD_X, 0);
        glm::vec3 objectY = selectedObject->modelMatrix() * glm::vec4(Transform::WORLD_Y, 0);
        glm::vec3 objectZ = selectedObject->modelMatrix() * glm::vec4(Transform::WORLD_Z, 0);

        switch (m_selectedPressed) {
            case static_cast<ID>(ReservedObjectID::TRANSFORM_ARROW_X): {
                /* Find if right vector is more aligned with the up or right of camera, to use the appropriate mouse diff */
                float cameraRightDot = glm::dot(objectX, cameraTransform.right());
                float cameraUpDot = glm::dot(objectX, cameraTransform.up());
                float movement;
                if (std::abs(cameraRightDot) > std::abs(cameraUpDot)) {
                    movement = ((cameraRightDot > 0) ? 1 : -1) * (float)mousePosDiff.x();
                } else {
                    movement = ((cameraUpDot > 0) ? 1 : -1) * ((float)-mousePosDiff.y());
                }
                position += selectedObjectTransform.X() * speed * movement;
                break;
            }
            case static_cast<ID>(ReservedObjectID::TRANSFORM_ARROW_Z): {
                /* Find if forward vector is more aligned with the up or right of camera, to use the appropriate mouse diff */
                float cameraRightDot = glm::dot(objectZ, cameraTransform.right());
                float cameraUpDot = glm::dot(objectZ, cameraTransform.up());
                float movement;
                if (std::abs(cameraRightDot) > std::abs(cameraUpDot)) {
                    movement = ((cameraRightDot > 0) ? 1 : -1) * (float)mousePosDiff.x();
                } else {
                    movement = ((cameraUpDot > 0) ? 1 : -1) * ((float)-mousePosDiff.y());
                }
                position += selectedObjectTransform.Z() * speed * movement;
                break;
            }
            case static_cast<ID>(ReservedObjectID::TRANSFORM_ARROW_Y): {
                /* Find if up vector is more aligned with the up or right of camera, to use the appropriate mouse diff */
                float cameraRightDot = glm::dot(objectY, cameraTransform.right());
                float cameraUpDot = glm::dot(objectY, cameraTransform.up());
                float movement;
                if (std::abs(cameraRightDot) > std::abs(cameraUpDot)) {
                    movement = ((cameraRightDot > 0) ? 1 : -1) * (float)mousePosDiff.x();
                } else {
                    movement = ((cameraUpDot > 0) ? 1 : -1) * ((float)-mousePosDiff.y());
                }
                position += selectedObjectTransform.Y() * speed * movement;
                break;
            }
            default:
                break;
        }
        selectedObjectTransform.position() = position;
        selectedObject->setLocalTransform(selectedObjectTransform);
        Q_EMIT selectedObjectPositionChanged();
    }
}

void VulkanViewportWindow::wheelEvent(QWheelEvent *ev)
{
    float delta = m_engine->delta();
    float speed = m_zoomSensitivity * delta;
    Transform &cameraTransform = m_engine->scene().camera()->transform();

    if (ev->angleDelta().y() > 0) {
        cameraTransform.position() += cameraTransform.forward() * static_cast<float>(ev->angleDelta().y()) * speed;
    } else if (ev->angleDelta().y() < 0) {
        cameraTransform.position() += cameraTransform.forward() * static_cast<float>(ev->angleDelta().y()) * speed;
    }
}

void VulkanViewportWindow::onUpdateCamera()
{
    /* FPS style camera movement */
    
    float speed = m_cameraDefaultSpeed;
    if (m_keysPressed[Qt::Key_Shift])
        speed = m_cameraFastSpeed;

    float finalSpeed = speed * m_engine->delta();
    Transform &cameraTransform = m_engine->scene().camera()->transform();
    cameraTransform.position() =
        cameraTransform.position() + static_cast<float>(m_keysPressed[Qt::Key_W]) * cameraTransform.forward() * finalSpeed;
    cameraTransform.position() =
        cameraTransform.position() - static_cast<float>(m_keysPressed[Qt::Key_S]) * cameraTransform.forward() * finalSpeed;
    cameraTransform.position() =
        cameraTransform.position() + static_cast<float>(m_keysPressed[Qt::Key_D]) * cameraTransform.right() * finalSpeed;
    cameraTransform.position() =
        cameraTransform.position() - static_cast<float>(m_keysPressed[Qt::Key_A]) * cameraTransform.right() * finalSpeed;
    cameraTransform.position() =
        cameraTransform.position() + static_cast<float>(m_keysPressed[Qt::Key_Q]) * cameraTransform.up() * finalSpeed;
    cameraTransform.position() =
        cameraTransform.position() - static_cast<float>(m_keysPressed[Qt::Key_E]) * cameraTransform.up() * finalSpeed;
}
