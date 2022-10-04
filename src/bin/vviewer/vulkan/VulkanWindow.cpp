#include "VulkanWindow.hpp"

#include <qevent.h>
#include <qapplication.h>

#include <zip.h>

VulkanWindow::VulkanWindow()
{
    m_scene = new VulkanScene(150);

    /*OrthographicCamera * camera = new OrthographicCamera();
    camera->setOrthoWidth(10.f);*/
    auto camera = std::make_shared<PerspectiveCamera>();
    camera->setFoV(60.0f);
    
    Transform cameraTransform;
    cameraTransform.setPosition(glm::vec3(1, 3, 10));
    cameraTransform.setRotation(glm::quat(glm::vec3(0, glm::radians(180.0f), 0)));
    camera->getTransform() = cameraTransform;
    
    m_scene->setCamera(camera);

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

Material* VulkanWindow::importMaterial(std::string name, std::string stackDirectory)
{
    Material* material = m_renderer->createMaterial(name, MaterialType::MATERIAL_PBR_STANDARD);
    if (material == nullptr) return nullptr;

    Texture* albedo = m_renderer->createTexture(stackDirectory + "/albedo.png", VK_FORMAT_R8G8B8A8_SRGB);
    Texture* ao = m_renderer->createTexture(stackDirectory + "/ao.png", VK_FORMAT_R8G8B8A8_UNORM);
    Texture* metallic = m_renderer->createTexture(stackDirectory + "/metallic.png", VK_FORMAT_R8G8B8A8_UNORM);
    Texture* normal = m_renderer->createTexture(stackDirectory + "/normal.png", VK_FORMAT_R8G8B8A8_UNORM);
    Texture* roughness = m_renderer->createTexture(stackDirectory + "/roughness.png", VK_FORMAT_R8G8B8A8_UNORM);

    if (albedo != nullptr) static_cast<MaterialPBRStandard*>(material)->setAlbedoTexture(albedo);
    if (ao != nullptr) static_cast<MaterialPBRStandard*>(material)->setAOTexture(ao);
    if (metallic != nullptr)  static_cast<MaterialPBRStandard*>(material)->setMetallicTexture(metallic);
    if (normal != nullptr)  static_cast<MaterialPBRStandard*>(material)->setNormalTexture(normal);
    if (roughness != nullptr) static_cast<MaterialPBRStandard*>(material)->setRoughnessTexture(roughness);

    return material;
}

Material* VulkanWindow::importZipMaterial(std::string name, std::string filename)
{
    struct zip_t *zip = zip_open(filename.c_str(), 0, 'r');

    /* Get .xtex file name */
    std::string xtexName;
    int i, n = zip_entries_total(zip);
    for (i = 0; i < n; ++i) {
        zip_entry_openbyindex(zip, i);
        {
            const std::string name = std::string(zip_entry_name(zip));
            auto pointPos = name.find(".");
            if (pointPos != std::string::npos && name.substr(pointPos) == ".xtex") {
                xtexName = name;
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

        uv *= glm::vec2(std::stof(X), std::stof(Y));
        zip_entry_close(zip);
    }

    /* Parse albedo */
    Texture * albedoTexture = nullptr;
    std::string albedoZipPath = "textures/albedo.png";
    if (zip_entry_open(zip, albedoZipPath.c_str()) == 0) {
        zip_entry_read(zip, &buf, &bufsize);

        int32_t x, y;
        stbi_uc *rawImgBuffer = stbi_load_from_memory(reinterpret_cast<const stbi_uc *>(buf), bufsize, &x, &y, nullptr, 4);

        Image<stbi_uc> image(rawImgBuffer, x, y, 4, false);
        std::string id = filename + ":" + albedoZipPath;
        albedoTexture = m_renderer->createTexture(id, &image, VK_FORMAT_R8G8B8A8_SRGB);
        if (albedoTexture == nullptr) {
            utils::ConsoleWarning("Unable to load albedo from zip texture stack");
        }
        /* entry_close will delete the memory */
        zip_entry_close(zip);
    }

    /* Parse roughness */
    Texture * roughnessTexture = nullptr;
    std::string roughnessZipPath = "textures/roughness.png";
    if (zip_entry_open(zip, roughnessZipPath.c_str()) == 0) {
        zip_entry_read(zip, &buf, &bufsize);

        int32_t x, y;
        stbi_uc *rawImgBuffer = stbi_load_from_memory(reinterpret_cast<const stbi_uc *>(buf), bufsize, &x, &y, nullptr, 4);

        Image<stbi_uc> image(rawImgBuffer, x, y, 4, false);
        std::string id = filename + ":" + roughnessZipPath;
        roughnessTexture = m_renderer->createTexture(id, &image, VK_FORMAT_R8G8B8A8_UNORM);
        if (roughnessTexture == nullptr) {
            utils::ConsoleWarning("Unable to load roughness from zip texture stack");
        }
        /* entry_close will delete the memory */
        zip_entry_close(zip);
    }

    /* Parse normal */
    Texture * normalTexture = nullptr;
    std::string normalZipPath = "textures/normal.png";
    if (zip_entry_open(zip, normalZipPath.c_str()) == 0) {
        zip_entry_read(zip, &buf, &bufsize);

        int32_t x, y;
        stbi_uc *rawImgBuffer = stbi_load_from_memory(reinterpret_cast<const stbi_uc *>(buf), bufsize, &x, &y, nullptr, 4);

        Image<stbi_uc> image(rawImgBuffer, x, y, 4, false);
        std::string id = filename + ":" + normalZipPath;
        normalTexture = m_renderer->createTexture(id, &image, VK_FORMAT_R8G8B8A8_UNORM);
        if (normalTexture == nullptr) {
            utils::ConsoleWarning("Unable to load normal from zip texture stack");
        }
        /* entry_close will delete the memory */
        zip_entry_close(zip);
    }

    MaterialPBRStandard* mat = dynamic_cast<MaterialPBRStandard*>(m_renderer->createMaterial(name, MaterialType::MATERIAL_PBR_STANDARD));
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
        QPointF pos = ev->position();
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
        m_mousePos = ev->position();
        m_mousePosFirst = false;
        return;
    }
 
    /* Calculate diff with the previous position */
    QPointF newMousePos = ev->position();
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
