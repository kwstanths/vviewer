#include "VulkanWindow.hpp"

#include <QPlatformSurfaceEvent>

#include <zip.h>

#include "vulkan/common/VulkanLimits.hpp"

using namespace vengine;

VulkanWindow::VulkanWindow() : QWindow()
{
    setSurfaceType(QSurface::SurfaceType::VulkanSurface);

    m_engine = new VulkanEngine();

    setVulkanInstance(m_engine->context().instance());

    /* Create default camera */
    auto camera = std::make_shared<PerspectiveCamera>();
    camera->setFoV(60.0f);
    
    Transform cameraTransform;
    cameraTransform.setPosition(glm::vec3(1, 3, 10));
    cameraTransform.setRotation(glm::quat(glm::vec3(0, glm::radians(1.F), 0)));
    camera->getTransform() = cameraTransform;
    
    m_engine->scene()->setCamera(camera);

    m_updateCameraTimer = new QTimer();
    m_updateCameraTimer->setInterval(16);
    connect(m_updateCameraTimer, SIGNAL(timeout()), this, SLOT(onUpdateCamera()));
    m_updateCameraTimer->start();
}

VulkanWindow::~VulkanWindow()
{

}

VulkanEngine * VulkanWindow::engine() const
{
    return m_engine;
}

void VulkanWindow::windowActicated(bool activated)
{
    if (activated) m_engine->start();
    else m_engine->stop();
}

void VulkanWindow::releaseResources()
{
    m_engine->releaseSwapChainResources();
    m_engine->releaseResources();    
}

void VulkanWindow::exposeEvent(QExposeEvent* event)
{
    if(isExposed() && !m_initialized)
    {
        /* Perform first time initialization the first time the surface window becomes available */
        VkSurfaceKHR surface = m_engine->context().instance()->surfaceForWindow(this);
        m_engine->setSurface(surface);
        
        m_engine->initResources();
        m_engine->initSwapChainResources(static_cast<uint32_t>(size().width()), static_cast<uint32_t>(size().height()));

        Q_EMIT initializationFinished();

        m_initialized = true;

        m_engine->start();
    }
}

bool VulkanWindow::event(QEvent* event) 
{
    switch(event->type())
    {
    case QEvent::PlatformSurface:
    {
        if (static_cast<QPlatformSurfaceEvent *>(event)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed)
        {
            m_engine->exit();
            m_engine->waitIdle();

            releaseResources();
        }
    }
    }

    return QWindow::event(event);
}

void VulkanWindow::resizeEvent(QResizeEvent *ev) 
{
    m_engine->scene()->getCamera()->setWindowSize(ev->size().width(), ev->size().height());

    VulkanSwapchain& swapchain = m_engine->swapchain();

    /* If the swapchain has been initialized, then we have to recreate it for the new size */
    if (swapchain.isInitialized())
    {
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

void VulkanWindow::keyPressEvent(QKeyEvent *ev) 
{
    m_keysPressed[ev->key()] = true;
}

void VulkanWindow::keyReleaseEvent(QKeyEvent *ev) 
{
    m_keysPressed[ev->key()] = false;
}

void VulkanWindow::mousePressEvent(QMouseEvent *ev) 
{
    if (ev->button() == Qt::LeftButton) {
        /* Select object from scene */
        QPointF pos = ev->position();
        QSize size = this->size();

        VulkanRenderer * renderer = static_cast<VulkanRenderer *>(m_engine->renderer());
        ID objectID = IDGeneration::fromRGB(renderer->selectObject(pos.x() / size.width(), pos.y() / size.height()));

        m_selectedPressed = objectID;
    }
}

void VulkanWindow::mouseReleaseEvent(QMouseEvent *ev) 
{
    std::shared_ptr<SceneObject> object = m_engine->scene()->getSceneObject(m_selectedPressed);
    m_selectedPressed = 0;

    if (object.get() == nullptr) return;

    Q_EMIT sceneObjectSelected(object);
}

void VulkanWindow::mouseMoveEvent(QMouseEvent *ev) 
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

    /* Perform camera movement if right button is pressed */
    float delta = m_engine->delta();
    Qt::MouseButtons buttons = ev->buttons();
    Transform& cameraTransform = m_engine->scene()->getCamera()->getTransform();
    if (buttons & Qt::RightButton) {
        float mouseSensitivity = 0.125f * delta;
        
        /* FPS style camera rotation, if middle mouse is pressed while the mouse is dragged over the window */
        glm::quat rotation = cameraTransform.getRotation();
        glm::quat qPitch = glm::angleAxis(-(float)mousePosDiff.y() * mouseSensitivity, glm::vec3(1, 0, 0));
        glm::quat qYaw = glm::angleAxis(-(float)mousePosDiff.x() * mouseSensitivity, glm::vec3(0, 1, 0));

        cameraTransform.setRotation(glm::normalize(qYaw * rotation * qPitch));
    }

    /* Perform movement of selected object if left button is pressed */
    VulkanRenderer * renderer = static_cast<VulkanRenderer *>(m_engine->renderer());
    float movementSensitivity = 1.25f * delta;
    if (buttons & Qt::LeftButton) {
        std::shared_ptr<SceneObject> selectedObject = renderer->getSelectedObject();
        if (selectedObject.get() == nullptr) return;

        Transform& selectedObjectTransform = selectedObject->m_localTransform;
        glm::vec3 position = selectedObjectTransform.getPosition();

        /* Get the basis vectors of the selected object */
        glm::vec3 objectX = selectedObject->m_modelMatrix * glm::vec4(Transform::X, 0);
        glm::vec3 objectY = selectedObject->m_modelMatrix * glm::vec4(Transform::Y, 0);
        glm::vec3 objectZ = selectedObject->m_modelMatrix * glm::vec4(Transform::Z, 0);
        
        switch (m_selectedPressed)
        {
        case static_cast<ID>(ReservedObjectID::RIGHT_TRANSFORM_ARROW):
        {
            /* Find if right vector is more aligned with the up or right of camera, to use the appropriate mouse diff */
            float cameraRightDot = glm::dot(objectX, cameraTransform.getRight());
            float cameraUpDot = glm::dot(objectX, cameraTransform.getUp());
            float movement;
            if (std::abs(cameraRightDot) > std::abs(cameraUpDot)) {
                movement = ((cameraRightDot > 0) ? 1 : -1) * (float)mousePosDiff.x();
            }
            else {
                movement = ((cameraUpDot > 0) ? 1 : -1) * ((float)-mousePosDiff.y());
            }
            position += selectedObjectTransform.getX() * movementSensitivity * movement;
            
            selectedObjectTransform.setPosition(position);
            Q_EMIT selectedObjectPositionChanged();
            break;
        }
        case static_cast<ID>(ReservedObjectID::FORWARD_TRANSFORM_ARROW):
        {
            /* Find if forward vector is more aligned with the up or right of camera, to use the appropriate mouse diff */
            float cameraRightDot = glm::dot(objectZ, cameraTransform.getRight());
            float cameraUpDot = glm::dot(objectZ, cameraTransform.getUp());
            float movement;
            if (std::abs(cameraRightDot) > std::abs(cameraUpDot)) {
                movement = ((cameraRightDot > 0) ? 1 : -1) * (float)mousePosDiff.x();
            }
            else {
                movement = ((cameraUpDot > 0) ? 1 : -1) * ((float)-mousePosDiff.y());
            }
            position += selectedObjectTransform.getZ() * movementSensitivity * movement;

            selectedObjectTransform.setPosition(position);
            Q_EMIT selectedObjectPositionChanged();
            break;
        }
        case static_cast<ID>(ReservedObjectID::UP_TRANSFORM_ARROW):
        {
            /* Find if up vector is more aligned with the up or right of camera, to use the appropriate mouse diff */
            float cameraRightDot = glm::dot(objectY, cameraTransform.getRight());
            float cameraUpDot = glm::dot(objectY, cameraTransform.getUp());
            float movement;
            if (std::abs(cameraRightDot) > std::abs(cameraUpDot)) {
                movement = ((cameraRightDot > 0) ? 1 : -1) * (float)mousePosDiff.x();
            }
            else {
                movement = ((cameraUpDot > 0) ? 1 : -1) * ((float)-mousePosDiff.y());
            }
            position += selectedObjectTransform.getY() * movementSensitivity * movement;

            selectedObjectTransform.setPosition(position);
            Q_EMIT selectedObjectPositionChanged();
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
    float cameraDefaultSpeed = 3.f;
    float cameraFastSpeed = 12.f;

    float speed = cameraDefaultSpeed;
    if (m_keysPressed[Qt::Key_Shift]) speed = cameraFastSpeed;

    float finalSpeed = speed * m_engine->delta();
    Transform& cameraTransform = m_engine->scene()->getCamera()->getTransform();
    cameraTransform.setPosition(cameraTransform.getPosition() + static_cast<float>(m_keysPressed[Qt::Key_W]) * cameraTransform.getForward() * finalSpeed);
    cameraTransform.setPosition(cameraTransform.getPosition() - static_cast<float>(m_keysPressed[Qt::Key_S]) * cameraTransform.getForward() * finalSpeed);
    cameraTransform.setPosition(cameraTransform.getPosition() + static_cast<float>(m_keysPressed[Qt::Key_D]) * cameraTransform.getRight() * finalSpeed);
    cameraTransform.setPosition(cameraTransform.getPosition() - static_cast<float>(m_keysPressed[Qt::Key_A]) * cameraTransform.getRight() * finalSpeed);
    cameraTransform.setPosition(cameraTransform.getPosition() + static_cast<float>(m_keysPressed[Qt::Key_Q]) * cameraTransform.getUp() * finalSpeed);
    cameraTransform.setPosition(cameraTransform.getPosition() - static_cast<float>(m_keysPressed[Qt::Key_E]) * cameraTransform.getUp() * finalSpeed);
}

std::shared_ptr<Material> VulkanWindow::importMaterial(std::string name, std::string stackDirectory)
{
    auto material = m_engine->materials().createMaterial(name, MaterialType::MATERIAL_PBR_STANDARD);
    if (material == nullptr) return nullptr;

    auto albedo = m_engine->textures().createTexture(stackDirectory + "/albedo.png", VK_FORMAT_R8G8B8A8_SRGB);
    auto ao = m_engine->textures().createTexture(stackDirectory + "/ao.png", VK_FORMAT_R8G8B8A8_UNORM);
    auto metallic = m_engine->textures().createTexture(stackDirectory + "/metallic.png", VK_FORMAT_R8G8B8A8_UNORM);
    auto normal = m_engine->textures().createTexture(stackDirectory + "/normal.png", VK_FORMAT_R8G8B8A8_UNORM);
    auto roughness = m_engine->textures().createTexture(stackDirectory + "/roughness.png", VK_FORMAT_R8G8B8A8_UNORM);

    if (albedo != nullptr) std::static_pointer_cast<MaterialPBRStandard>(material)->setAlbedoTexture(albedo);
    if (ao != nullptr) std::static_pointer_cast<MaterialPBRStandard>(material)->setAOTexture(ao);
    if (metallic != nullptr) std::static_pointer_cast<MaterialPBRStandard>(material)->setMetallicTexture(metallic);
    if (normal != nullptr) std::static_pointer_cast<MaterialPBRStandard>(material)->setNormalTexture(normal);
    if (roughness != nullptr) std::static_pointer_cast<MaterialPBRStandard>(material)->setRoughnessTexture(roughness);

    return material;
}

std::shared_ptr<Material> VulkanWindow::importZipMaterial(std::string name, std::string filename)
{
    VulkanRenderer * renderer = static_cast<VulkanRenderer *>(m_engine->renderer());

    struct zip_t *zip = zip_open(filename.c_str(), 0, 'r');

    /* Get .xtex file name */
    std::string xtexName;
    std::string texturesFolder = "";
    int i, n = zip_entries_total(zip);
    for (i = 0; i < n; ++i) {
        zip_entry_openbyindex(zip, i);
        {
            const std::string name = std::string(zip_entry_name(zip));
            auto pointPos = name.find(".");
            if (pointPos != std::string::npos && name.substr(pointPos) == ".xtex") {
                xtexName = name;
                
                auto internalFolderIndex = name.rfind("/");
                if (internalFolderIndex != std::string::npos)
                {
                    texturesFolder = name.substr(0, internalFolderIndex) + "/";

                }
                break;
            }
        }
        zip_entry_close(zip);
    }

    void *buf = NULL;
    size_t bufsize;

    /* Parse .xtex file paremeters */
    glm::vec2 uv(1.F);
    if (zip_entry_open(zip, xtexName.c_str()) == 0) {
        zip_entry_read(zip, &buf, &bufsize);

        std::string testT((char *)buf);

        const std::string X =
            testT.substr(0, testT.find("</width>")).substr(testT.find("<width>") + std::string("<width>").length());
        const std::string Y =
            testT.substr(0, testT.find("</height>")).substr(testT.find("<height>") + std::string("<height>").length());

        uv *= glm::vec2(100.F / std::stof(X), 100.F / std::stof(Y));
        zip_entry_close(zip);
    }

    /* Parse albedo */
    std::shared_ptr<Texture> albedoTexture = nullptr;
    std::string albedoZipPath = texturesFolder + "textures/albedo.png";
    if (zip_entry_open(zip, albedoZipPath.c_str()) == 0) {
        zip_entry_read(zip, &buf, &bufsize);

        int32_t x, y;
        stbi_uc *rawImgBuffer = stbi_load_from_memory(reinterpret_cast<const stbi_uc *>(buf), bufsize, &x, &y, nullptr, 4);

        auto image = std::make_shared<Image<stbi_uc>>(rawImgBuffer, x, y, 4, false);
        std::string id = filename + ":" + albedoZipPath;
        albedoTexture = m_engine->textures().createTexture(id, image, VK_FORMAT_R8G8B8A8_SRGB);
        if (albedoTexture == nullptr) {
            debug_tools::ConsoleWarning("Unable to load albedo from zip texture stack");
        }
        /* entry_close will delete the memory */
        zip_entry_close(zip);
    }

    /* Parse roughness */
    std::shared_ptr<Texture> roughnessTexture = nullptr;
    std::string roughnessZipPath = texturesFolder + "textures/roughness.png";
    if (zip_entry_open(zip, roughnessZipPath.c_str()) == 0) {
        zip_entry_read(zip, &buf, &bufsize);

        int32_t x, y;
        stbi_uc *rawImgBuffer = stbi_load_from_memory(reinterpret_cast<const stbi_uc *>(buf), bufsize, &x, &y, nullptr, 4);

        auto image = std::make_shared<Image<stbi_uc>>(rawImgBuffer, x, y, 4, false);
        std::string id = filename + ":" + roughnessZipPath;
        roughnessTexture = m_engine->textures().createTexture(id, image, VK_FORMAT_R8G8B8A8_UNORM);
        if (roughnessTexture == nullptr) {
            debug_tools::ConsoleWarning("Unable to load roughness from zip texture stack");
        }
        /* entry_close will delete the memory */
        zip_entry_close(zip);
    }

    /* Parse normal */
    std::shared_ptr<Texture> normalTexture = nullptr;
    std::string normalZipPath = texturesFolder + "textures/normal.png";
    if (zip_entry_open(zip, normalZipPath.c_str()) == 0) {
        zip_entry_read(zip, &buf, &bufsize);

        int32_t x, y;
        stbi_uc *rawImgBuffer = stbi_load_from_memory(reinterpret_cast<const stbi_uc *>(buf), bufsize, &x, &y, nullptr, 4);

        auto image = std::make_shared<Image<stbi_uc>>(rawImgBuffer, x, y, 4, false);
        std::string id = filename + ":" + normalZipPath;
        normalTexture = m_engine->textures().createTexture(id, image, VK_FORMAT_R8G8B8A8_UNORM);
        if (normalTexture == nullptr) {
            debug_tools::ConsoleWarning("Unable to load normal from zip texture stack");
        }
        /* entry_close will delete the memory */
        zip_entry_close(zip);
    }

    auto mat = std::dynamic_pointer_cast<MaterialPBRStandard>(m_engine->materials().createMaterial(name, MaterialType::MATERIAL_PBR_STANDARD));
    mat->setAlbedoTexture(albedoTexture);
    mat->setRoughnessTexture(roughnessTexture);
    mat->roughness() = 1.0F;
    mat->setNormalTexture(normalTexture);
    mat->metallic() = 0.0F;
    mat->uTiling() = uv.x;
    mat->vTiling() = uv.y;
    mat->m_path = filename;

    return mat;
}