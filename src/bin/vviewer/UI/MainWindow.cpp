#include "MainWindow.hpp"

#include <qvulkaninstance.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qmenubar.h>
#include <qfiledialog.h>
#include <QVariant>
#include <glm/glm.hpp>

#include <utils/Console.hpp>

#include "DialogAddSceneObject.hpp"
#include "DialogCreateMaterial.hpp"
#include "DialogSceneExport.hpp"
#include "WidgetMaterial.hpp"
#include "UIUtils.hpp"

#include "core/SceneImport.hpp"

MainWindow::MainWindow(QWidget *parent) :QMainWindow(parent) {
    
    QWidget * widgetVulkan = initVulkanWindowWidget();

    QHBoxLayout * layout_main = new QHBoxLayout();
    layout_main->addWidget(initLeftPanel());
    layout_main->addWidget(widgetVulkan);
    layout_main->addWidget(initControlsWidget());

    QWidget * widget_main = new QWidget();
    widget_main->setLayout(layout_main);

    m_scene = m_vulkanWindow->m_scene;

    createMenu();

    setCentralWidget(widget_main);
    resize(1600, 1000);
}

MainWindow::~MainWindow() {

}

QWidget * MainWindow::initLeftPanel()
{

    m_sceneGraphWidget = new QTreeWidget(this);
    m_sceneGraphWidget->setStyleSheet("background-color: rgba(240, 240, 240, 255);");
    connect(m_sceneGraphWidget, &QTreeWidget::itemSelectionChanged, this, &MainWindow::onSelectedSceneObjectChangedSlotUI);
    
    m_sceneGraphWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_sceneGraphWidget, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(onContextMenuSceneGraph(const QPoint&)));

    m_sceneGraphWidget->setHeaderLabel("Scene:");
    m_sceneGraphWidget->setFixedWidth(200);
    return m_sceneGraphWidget;
}

QWidget * MainWindow::initVulkanWindowWidget()
{
    const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
    };

    QByteArrayList layers;
    QByteArrayList extensions;

    /* Debug layers and extensions */
#ifdef NDEBUG

#else
    layers.append("VK_LAYER_KHRONOS_validation");
    extensions.append("VK_EXT_debug_utils");
#endif

    extensions.append("VK_KHR_get_physical_device_properties2");

    m_vulkanInstance = new QVulkanInstance();
    m_vulkanInstance->setLayers(layers);
    m_vulkanInstance->setExtensions(extensions);
    m_vulkanInstance->setApiVersion(QVersionNumber(1, 2));

    if (!m_vulkanInstance->create()) {
        utils::Console(utils::DebugLevel::FATAL, "Failed to create Vulkan instance: " +
            std::to_string(m_vulkanInstance->errorCode()));
    }

    QVulkanInfoVector<QVulkanLayer> supportedLayers = m_vulkanInstance->supportedLayers();
    for (auto layer : layers) {
        if (!supportedLayers.contains(layer)) {
            utils::Console(utils::DebugLevel::WARNING, "Layer not found: " + std::string(layer));
        }
    }

    QVulkanInfoVector<QVulkanExtension> supportedExtensions = m_vulkanInstance->supportedExtensions();
    for (auto extension : extensions) {
        if (!supportedExtensions.contains(extension)) {
            utils::Console(utils::DebugLevel::WARNING, "Extension not found: " + std::string(extension));
        }
    }

    m_vulkanWindow = new VulkanWindow();
    m_vulkanWindow->setVulkanInstance(m_vulkanInstance);
    connect(m_vulkanWindow, &VulkanWindow::sceneObjectSelected, this, &MainWindow::onSelectedSceneObjectChangedSlot3DScene);

    return QWidget::createWindowContainer(m_vulkanWindow);
}

QWidget * MainWindow::initControlsWidget()
{
    m_layoutControls = new QVBoxLayout();
    m_layoutControls->setAlignment(Qt::AlignTop);
    QWidget * widget_controls = new QWidget();
    widget_controls->setLayout(m_layoutControls);
    widget_controls->setFixedWidth(280);

    Transform lightTransform;
    lightTransform.setRotationEuler(0, glm::radians(180.0f), 0);
    auto light = std::make_shared<DirectionalLight>(lightTransform, glm::vec3(1, 0.9, 0.8), 1.F);
    m_vulkanWindow->m_scene->setDirectionalLight(light);
    
    m_widgetEnvironment = new WidgetEnvironment(nullptr, m_vulkanWindow->m_scene);

    QTabWidget* widget_tab = new QTabWidget();
    widget_tab->insertTab(0, widget_controls, "Scene object");
    widget_tab->insertTab(1, m_widgetEnvironment, "Environment");

    widget_tab->setFixedWidth(300);
    return widget_tab;
}

void MainWindow::createMenu()
{
    QAction * actionImportModel = new QAction(tr("&Import a model"), this);
    actionImportModel->setStatusTip(tr("Import a model"));
    connect(actionImportModel, &QAction::triggered, this, &MainWindow::onImportModelSlot);
    QAction * actionImportColorTexture = new QAction(tr("&Import color textures"), this);
    actionImportColorTexture->setStatusTip(tr("Import color textures"));
    connect(actionImportColorTexture, &QAction::triggered, this, &MainWindow::onImportTextureColorSlot);
    QAction * actionImportOtherTexture = new QAction(tr("&Import other textures"), this);
    actionImportOtherTexture->setStatusTip(tr("Import other textures"));
    connect(actionImportOtherTexture, &QAction::triggered, this, &MainWindow::onImportTextureOtherSlot);
    QAction * actionImportHDRTexture = new QAction(tr("&Import HDR texture"), this);
    actionImportHDRTexture->setStatusTip(tr("Import HDR texture"));
    connect(actionImportHDRTexture, &QAction::triggered, this, &MainWindow::onImportTextureHDRSlot);
    QAction* actionImportEnvironmentMap = new QAction(tr("&Import environment map"), this);
    actionImportEnvironmentMap->setStatusTip(tr("Import environment map"));
    connect(actionImportEnvironmentMap, &QAction::triggered, this, &MainWindow::onImportEnvironmentMap);
    QAction* actionImportMaterial = new QAction(tr("&Import material"), this);
    actionImportMaterial->setStatusTip(tr("Import material"));
    connect(actionImportMaterial, &QAction::triggered, this, &MainWindow::onImportMaterial);
    QAction* actionImportScene = new QAction(tr("&Import scene"), this);
    actionImportScene->setStatusTip(tr("Import scene"));
    connect(actionImportScene, &QAction::triggered, this, &MainWindow::onImportScene);

    QAction * addSceneObjectRootSlot = new QAction(tr("&Add a scene object at root"), this);
    addSceneObjectRootSlot->setStatusTip(tr("Add a scene object at root"));
    connect(addSceneObjectRootSlot, &QAction::triggered, this, &MainWindow::onAddSceneObjectRootSlot);
    QAction * actionCreateMaterial = new QAction(tr("&Add a material"), this);
    actionCreateMaterial->setStatusTip(tr("Add a material"));
    connect(actionCreateMaterial, &QAction::triggered, this, &MainWindow::onCreateMaterialSlot);
    QAction * actionCreatePointLight = new QAction(tr("&Add a point light"), this);
    actionCreatePointLight->setStatusTip(tr("Add point light"));
    connect(actionCreatePointLight, &QAction::triggered, this, &MainWindow::onAddPointLightRootSlot);

    QAction* actionRender = new QAction(tr("&Render scene (GPU) WIP"), this);
    actionRender->setStatusTip(tr("Render scene"));
    connect(actionRender, &QAction::triggered, this, &MainWindow::onRenderSceneSlot);

    QAction* actionExport = new QAction(tr("&Export scene"), this);
    actionExport->setStatusTip(tr("EXport scene"));
    connect(actionExport, &QAction::triggered, this, &MainWindow::onExportSceneSlot);

    QMenu * m_menuImport = menuBar()->addMenu(tr("&Import"));
    m_menuImport->addAction(actionImportModel);
    m_menuImport->addAction(actionImportColorTexture);
    m_menuImport->addAction(actionImportOtherTexture);
    m_menuImport->addAction(actionImportHDRTexture);
    m_menuImport->addAction(actionImportEnvironmentMap);
    m_menuImport->addAction(actionImportMaterial);
    m_menuImport->addAction(actionImportScene);
    QMenu * m_menuAdd = menuBar()->addMenu(tr("&Add"));
    m_menuAdd->addAction(addSceneObjectRootSlot);
    m_menuAdd->addAction(actionCreateMaterial);
    m_menuAdd->addAction(actionCreatePointLight);
    QMenu* m_menuRender = menuBar()->addMenu(tr("&Render"));
    m_menuRender->addAction(actionRender);
    QMenu* m_menuExport = menuBar()->addMenu(tr("&Export"));
    m_menuExport->addAction(actionExport);
}

QTreeWidgetItem* MainWindow::createTreeWidgetItem(std::shared_ptr<SceneObject> object)
{
    QTreeWidgetItem* item = new QTreeWidgetItem({ QString(object->m_name.c_str()) });
    QVariant data;
    data.setValue(object);
    item->setData(0, Qt::UserRole, data);
    m_treeWidgetItems.insert({ object->getID(), item });
    return item;
}

void MainWindow::selectObject(QTreeWidgetItem* selectedItem)
{
    /* If it's the previously selected item, return */
    if (selectedItem == m_selectedPreviousWidgetItem) return;
    m_sceneGraphWidget->blockSignals(true);

    /* When the selected object on the scene list changes, remove previous widgets from the controls, and add new */
    clearControlsUI();

    /* Get selected object */
    std::shared_ptr<SceneObject> sceneObject = selectedItem->data(0, Qt::UserRole).value<std::shared_ptr<SceneObject>>();
    /* Remove UI selection if needed */
    if (!selectedItem->isSelected()) {
        selectedItem->setSelected(true);
        selectedItem->setExpanded(true);
    }

    /* Remove selection from previously selected item */
    if (m_selectedPreviousWidgetItem != nullptr) {
        m_selectedPreviousWidgetItem->setSelected(false);
        std::shared_ptr<SceneObject> previousSelected = m_selectedPreviousWidgetItem->data(0, Qt::UserRole).value<std::shared_ptr<SceneObject>>();
        previousSelected->m_isSelected = false;
    }

    /* Set selection to new item */
    m_selectedPreviousWidgetItem = selectedItem;
    sceneObject->m_isSelected = true;
    m_vulkanWindow->m_renderer->setSelectedObject(sceneObject);

    /* Create UI elements for its components, connect them to slots, and add them to the controls widget */
    m_selectedObjectWidgetName = new WidgetName(nullptr, QString(sceneObject->m_name.c_str()));
    connect(m_selectedObjectWidgetName->m_text, &QTextEdit::textChanged, this, &MainWindow::onSelectedSceneObjectNameChangedSlot);

    m_selectedObjectWidgetTransform = new WidgetTransform(nullptr, sceneObject);
    connect(m_vulkanWindow, &VulkanWindow::selectedObjectPositionChanged, m_selectedObjectWidgetTransform, &WidgetTransform::onPositionChangedFrom3DScene);

    m_layoutControls->addWidget(m_selectedObjectWidgetName);
    m_layoutControls->addWidget(m_selectedObjectWidgetTransform);

    if (sceneObject->has(ComponentType::MESH))
    {
        m_selectedObjectWidgetMeshModel = new WidgetMeshModel(nullptr, sceneObject.get());
        m_layoutControls->addWidget(m_selectedObjectWidgetMeshModel);
    }

    if (sceneObject->has(ComponentType::MATERIAL)) 
    {
        m_selectedObjectWidgetMaterial = new WidgetMaterial(nullptr, sceneObject.get());
        m_layoutControls->addWidget(m_selectedObjectWidgetMaterial);
    }

    if (sceneObject->has(ComponentType::POINT_LIGHT)) 
    {
        m_selectedObjectWidgetPointLight = new WidgetPointLight(nullptr, sceneObject->get<PointLight*>(ComponentType::POINT_LIGHT));
        m_layoutControls->addWidget(m_selectedObjectWidgetPointLight);
    }

    m_sceneGraphWidget->blockSignals(false);
}

void MainWindow::removeObjectFromScene(QTreeWidgetItem* treeItem)
{
    std::shared_ptr<SceneObject> selectedObject = treeItem->data(0, Qt::UserRole).value<std::shared_ptr<SceneObject>>();

    m_scene->removeSceneObject(selectedObject);

    m_vulkanWindow->m_renderer->setSelectedObject(nullptr);
    m_selectedPreviousWidgetItem = nullptr;

    /* Remove from UI */
    if (selectedObject->m_parent == nullptr) {
        m_sceneGraphWidget->removeItemWidget(treeItem, 0);
        delete treeItem;
    }
    else {
        treeItem->parent()->removeChild(treeItem);
        delete treeItem;
    }
}

void MainWindow::addSceneObjectMeshes(QTreeWidgetItem * parentItem, std::string modelName, std::string material)
{
    AssetManager<std::string, MeshModel*>& instanceModels = AssetManager<std::string, MeshModel*>::getInstance();
    if (!instanceModels.isPresent(modelName))
    {
        utils::ConsoleWarning("VulkanScene::CreateObject(): Mesh model " + modelName + " is not imported");
        return;
    }
    AssetManager<std::string, Material*>& instanceMaterials = AssetManager<std::string, Material*>::getInstance();
    if (!instanceMaterials.isPresent(material)) 
    {
        utils::ConsoleWarning("VulkanScene::CreateObject(): Material " + material + " is not created");
        return;
    }

    MeshModel* meshhModel = instanceModels.Get(modelName);
    std::vector<Mesh*> modelMeshes = meshhModel->getMeshes();
    
    std::shared_ptr<SceneObject> parentObject = parentItem->data(0, Qt::UserRole).value<std::shared_ptr<SceneObject>>();

    /* For all meshes inside the mesh model, add then in the scene and the UI */
    for (auto& m : modelMeshes) {
        std::shared_ptr<SceneObject> child = m_scene->addSceneObject(m->m_name, parentObject, Transform());
        child->assign(m);
        child->assign(instanceMaterials.Get(material));

        QTreeWidgetItem* childItem = createTreeWidgetItem(child);

        parentItem->addChild(childItem);
    }
}

void MainWindow::clearControlsUI()
{
    if (m_selectedObjectWidgetName != nullptr) {
        delete m_selectedObjectWidgetName;
        m_selectedObjectWidgetName = nullptr;
    }
    if (m_selectedObjectWidgetTransform != nullptr) {
        delete m_selectedObjectWidgetTransform;
        m_selectedObjectWidgetTransform = nullptr;
    }
    if (m_selectedObjectWidgetMeshModel != nullptr) {
        delete m_selectedObjectWidgetMeshModel;
        m_selectedObjectWidgetMeshModel = nullptr;
    }
    if (m_selectedObjectWidgetMaterial != nullptr) {
        delete m_selectedObjectWidgetMaterial;
        m_selectedObjectWidgetMaterial = nullptr;
    }
    if (m_selectedObjectWidgetPointLight != nullptr) {
        delete m_selectedObjectWidgetPointLight;
        m_selectedObjectWidgetPointLight = nullptr;
    }
}

void MainWindow::onImportModelSlot()
{   
    QString filename = QFileDialog::getOpenFileName(this,
        tr("Import model"), "./assets/models",
        tr("Model (*.obj *.fbx);;All Files (*)"));

    if (filename == "") return;

    bool ret = m_vulkanWindow->m_renderer->createVulkanMeshModel(filename.toStdString());
 
    if (ret) {
        utils::ConsoleInfo("Model imported");
    }
}

void MainWindow::onImportTextureColorSlot()
{
    QStringList filenames = QFileDialog::getOpenFileNames(this,
        tr("Import textures"), "./assets",
        tr("Textures (*.tga, *.png);;All Files (*)"));

    for (const auto& texture : filenames)
    {
        Texture * tex = m_vulkanWindow->m_renderer->createTexture(texture.toStdString(), VK_FORMAT_R8G8B8A8_SRGB);
        if (tex) {
            utils::ConsoleInfo("Texture: " + texture.toStdString() + " imported");
        }
    }
}

void MainWindow::onImportTextureOtherSlot()
{
    QStringList filenames = QFileDialog::getOpenFileNames(this,
        tr("Import textures"), "./assets",
        tr("Textures (*.tga, *.png);;All Files (*)"));

    for (const auto& texture : filenames)
    {
        Texture * tex = m_vulkanWindow->m_renderer->createTexture(texture.toStdString(), VK_FORMAT_R8G8B8A8_UNORM);
        if (tex) {
            utils::ConsoleInfo("Texture: " + texture.toStdString() + " imported");
        }
    }
}

void MainWindow::onImportTextureHDRSlot()
{
    QString filename = QFileDialog::getOpenFileName(this,
        tr("Import HDR texture"), "./assets/HDR/",
        tr("Model (*.hdr);;All Files (*)"));

    if (filename == "") return;

    Texture * tex = m_vulkanWindow->m_renderer->createTextureHDR(filename.toStdString());
    if (tex) {
        utils::ConsoleInfo("Texture: " + filename.toStdString() + " imported");
        m_widgetEnvironment->updateMaps();
    }
}

void MainWindow::onImportEnvironmentMap()
{
    QString filename = QFileDialog::getOpenFileName(this,
        tr("Import HDR texture"), "./assets/HDR/",
        tr("Model (*.hdr);;All Files (*)"));

    if (filename == "") return;

    EnvironmentMap* envMap = m_vulkanWindow->m_renderer->createEnvironmentMap(filename.toStdString());
    if (envMap) {
        utils::ConsoleInfo("Environment map: " + filename.toStdString() + " imported");
        m_widgetEnvironment->updateMaps();
    }
}

void MainWindow::onImportMaterial()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Choose directory", "./assets/materials/", QFileDialog::ShowDirsOnly);
    if (dir == "") return;

    std::string materialName = dir.split('/').back().toStdString();
    std::string dirStd = dir.toStdString();

    m_vulkanWindow->importMaterial(materialName, dirStd);
}

void MainWindow::onImportScene()
{
    /* Select json file */
    QString sceneFile = QFileDialog::getOpenFileName(this,
        tr("Import scene"), "./assets/",
        tr("Model (*.json);;All Files (*)"));

    if (sceneFile == "") return;

    /* Parse json file */
    ImportedSceneCamera camera; 
    std::vector<ImportedSceneMaterial> materials;
    ImportedSceneEnvironment env;
    std::vector<ImportedSceneObject> sceneObjects;
    std::string sceneFolder;
    try{
        sceneFolder = importScene(sceneFile.toStdString(), camera, materials, env, sceneObjects);
    } catch (std::runtime_error& e) {
        utils::ConsoleWarning("Unable to open scene file: " + std::string(e.what()));
        return;
    }

    /* Set camera */
    {
        auto newCamera = std::make_shared<PerspectiveCamera>();
        Transform& cameraTransform = newCamera->getTransform();
        cameraTransform.setPosition(camera.position);
        cameraTransform.setRotation(glm::normalize(camera.target - camera.position), camera.up);

        newCamera->setFoV(camera.fov);
        newCamera->setWindowSize(m_vulkanWindow->size().width(), m_vulkanWindow->size().height());

        m_scene->setCamera(newCamera);
        m_widgetEnvironment->setCamera(newCamera);
    }

    /* Create materials */
    utils::ConsoleInfo("Importing materials...");
    for (auto& m : materials) {
        if (m.type == ImportedSceneMaterialType::STACK)
        {
            m_vulkanWindow->importMaterial(m.name, sceneFolder + m.stackDir);
        }
        else if (m.type == ImportedSceneMaterialType::DIFFUSE)
        {
            MaterialLambert* mat = dynamic_cast<MaterialLambert*>(m_vulkanWindow->m_renderer->createMaterial(m.name, MaterialType::MATERIAL_LAMBERT));

            if (mat == nullptr) continue;

            mat->albedo() = glm::vec4(m.albedoValue, 1);
            if (m.albedoTexture != "") {
                Texture* tex = m_vulkanWindow->m_renderer->createTexture(sceneFolder + m.albedoTexture, VK_FORMAT_R8G8B8A8_SRGB);
                mat->setAlbedoTexture(tex);
            }
        } 
        else if (m.type == ImportedSceneMaterialType::DISNEY)
        {
            MaterialPBRStandard* mat = dynamic_cast<MaterialPBRStandard*>(m_vulkanWindow->m_renderer->createMaterial(m.name, MaterialType::MATERIAL_PBR_STANDARD));

            if (mat == nullptr) continue;

            mat->albedo() = glm::vec4(m.albedoValue, 1);
            if (m.albedoTexture != "") {
                Texture* tex = m_vulkanWindow->m_renderer->createTexture(sceneFolder + m.albedoTexture, VK_FORMAT_R8G8B8A8_SRGB);
                if (tex != nullptr) mat->setAlbedoTexture(tex);
            }
            mat->roughness() = m.roughnessValue;
            if (m.roughnessTexture != "") {
                Texture* tex = m_vulkanWindow->m_renderer->createTexture(sceneFolder + m.roughnessTexture, VK_FORMAT_R8G8B8A8_UNORM);
                if (tex != nullptr) mat->setRoughnessTexture(tex);
            }
            mat->metallic() = m.metallicValue;
            if (m.metallicTexture != "") {
                Texture* tex = m_vulkanWindow->m_renderer->createTexture(sceneFolder + m.metallicTexture, VK_FORMAT_R8G8B8A8_UNORM);
                if (tex != nullptr) mat->setMetallicTexture(tex);
            }
            if (m.normalTexture != "") {
                Texture* tex = m_vulkanWindow->m_renderer->createTexture(sceneFolder + m.normalTexture, VK_FORMAT_R8G8B8A8_UNORM);
                if (tex != nullptr) mat->setNormalTexture(tex);
            }
        }
    }

    /* Create and set the environment map */
    if (env.path != "") {
        EnvironmentMap* envMap = m_vulkanWindow->m_renderer->createEnvironmentMap(sceneFolder + env.path);
        if (envMap) {
            utils::ConsoleInfo("Environment map: " + sceneFolder + env.path + " set");
            m_widgetEnvironment->updateMaps();

            AssetManager<std::string, MaterialSkybox*>& materials = AssetManager<std::string, MaterialSkybox*>::getInstance();
            MaterialSkybox* material = materials.Get("skybox");

            material->setMap(envMap);
        }
    }

    /* Remove old scene */
    while(m_sceneGraphWidget->topLevelItemCount() != 0)
    {
        removeObjectFromScene(m_sceneGraphWidget->topLevelItem(0));
    }
    m_selectedPreviousWidgetItem = nullptr;

    /* Create new scene */
    /* Create a top level item, and add everything below it */
    std::shared_ptr<SceneObject> object = m_scene->addSceneObject("New object (" + std::to_string(m_nObjects++) + ")", Transform());
    QTreeWidgetItem* parentItem = createTreeWidgetItem(object);
    m_sceneGraphWidget->addTopLevelItem(parentItem);

    utils::ConsoleInfo("Importing models...");
    for (auto& o : sceneObjects) 
    {
        bool ret = m_vulkanWindow->m_renderer->createVulkanMeshModel(sceneFolder + o.path);
        if (ret) {
            utils::ConsoleInfo(sceneFolder + o.path + " imported");
        }

        Transform t;
        t.setPosition(o.position);
        t.setScale(o.scale);
        t.setRotationEuler(glm::radians(o.rotation.x), glm::radians(o.rotation.y), glm::radians(o.rotation.z));

        addSceneObjectMeshes(parentItem, sceneFolder + o.path, o.material);
    }

    selectObject(parentItem);

    utils::ConsoleInfo(sceneFile.toStdString() + " imported");
}

void MainWindow::onAddSceneObjectRootSlot()
{
    DialogAddSceneObject * dialog = new DialogAddSceneObject(nullptr, "Add an object to the scene", getImportedModels(), getCreatedMaterials());
    dialog->exec();

    std::string selectedModel = dialog->getSelectedModel();
    if (selectedModel == "") return;

    /* Create parent oject */
    /* TODO set a some other way name */
    std::shared_ptr<SceneObject> object = m_scene->addSceneObject("New object (" + std::to_string(m_nObjects++) + ")", dialog->getTransform());
    QTreeWidgetItem* parentItem = createTreeWidgetItem(object);

    /* Add mesh model meshes as children of this item */
    addSceneObjectMeshes(parentItem, selectedModel, dialog->getSelectedMaterial());

    m_sceneGraphWidget->addTopLevelItem(parentItem);
}

void MainWindow::onAddSceneObjectSlot()
{
    /* Get currently selected tree item */
    QTreeWidgetItem* selectedItem = m_sceneGraphWidget->currentItem();
    /* If no item is selected, add scene object at root */
    if (selectedItem == nullptr) {
        onAddSceneObjectRootSlot();
        return;
    }

    DialogAddSceneObject* dialog = new DialogAddSceneObject(nullptr, "Add an object to the scene", getImportedModels(), getCreatedMaterials());
    dialog->exec();
    std::string selectedModel = dialog->getSelectedModel();
    if (selectedModel == "") return;

    std::shared_ptr<SceneObject> parentObject = selectedItem->data(0, Qt::UserRole).value<std::shared_ptr<SceneObject>>();

    /* Create parent oject */
    /* TODO set a some other way name */
    std::shared_ptr<SceneObject> object = m_scene->addSceneObject("New object (" + std::to_string(m_nObjects++) + ")", parentObject, dialog->getTransform());
    QTreeWidgetItem* parentItem = createTreeWidgetItem(object);

    /* Add mesh model meshes ad children on this item */
    addSceneObjectMeshes(parentItem, selectedModel, dialog->getSelectedMaterial());

    selectedItem->addChild(parentItem);
}

void MainWindow::onRemoveSceneObjectSlot()
{
    /* TODO remove scene objects from m_treeWidgetItems, resursively */

    /* Get the currently selected tree item */
    QTreeWidgetItem* selectedItem = m_sceneGraphWidget->currentItem();
    removeObjectFromScene(selectedItem);
}

void MainWindow::onCreateMaterialSlot()
{
    DialogCreateMaterial * dialog = new DialogCreateMaterial(nullptr, "Create a material", getImportedModels());
    dialog->exec();

    std::string selectedModel = dialog->m_selectedName.toStdString();
    if (dialog->m_selectedMaterialType == MaterialType::MATERIAL_NOT_SET) return;

    std::string materialName = dialog->m_selectedName.toStdString();
    if (materialName == "") {
        utils::ConsoleWarning("Material name can't be empty");
        return;
    }

    Material* material = m_vulkanWindow->m_renderer->createMaterial(materialName, dialog->m_selectedMaterialType);
    if (material == nullptr) {
        utils::ConsoleWarning("Failed to create material");
    }
}

void MainWindow::onAddPointLightRootSlot()
{
    std::shared_ptr<SceneObject> sceneObject = m_scene->addSceneObject("Point light", Transform());
    PointLight * light = new PointLight({1, 1, 1}, 1.F);
    sceneObject->assign(light);
    
    QTreeWidgetItem* item = createTreeWidgetItem(sceneObject);

    m_sceneGraphWidget->addTopLevelItem(item);
}

void MainWindow::onAddPointLightSlot()
{
    /* Get currently selected tree item */
    QTreeWidgetItem* selectedItem = m_sceneGraphWidget->currentItem();
    /* If no item is selected, add scene object at root */
    if (selectedItem == nullptr) {
        onAddPointLightRootSlot();
        return;
    }
    std::shared_ptr<SceneObject> selected = selectedItem->data(0, Qt::UserRole).value<std::shared_ptr<SceneObject>>();

    std::shared_ptr<SceneObject> sceneObject = m_scene->addSceneObject("Point light", selected, Transform());
    PointLight * light = new PointLight({1, 1, 1}, 1.F);
    sceneObject->assign(light);

    QTreeWidgetItem* item = createTreeWidgetItem(sceneObject);

    selectedItem->addChild(item);
}

void MainWindow::onRenderSceneSlot()
{
    m_vulkanWindow->m_renderer->renderRT();
}

void MainWindow::onExportSceneSlot()
{
    DialogSceneExport* dialog = new DialogSceneExport(nullptr);
    dialog->exec();

    std::string folderName = dialog->getDestinationFolderName();
    if (folderName == "") return;

    m_scene->exportScene(dialog->getDestinationFolderName(), dialog->getResolutionWidth(), dialog->getResolutionHeight(), dialog->getSamples());

    delete dialog;
}

void MainWindow::onSelectedSceneObjectChangedSlotUI()
{
    /* Get currently selected object, and the corresponding SceneObject */
    QTreeWidgetItem * selectedItem = m_sceneGraphWidget->currentItem();
    /* When all items have been removed, this function is called with a null object selected */
    if (selectedItem == nullptr) return;

    selectObject(selectedItem);
}

void MainWindow::onSelectedSceneObjectChangedSlot3DScene(std::shared_ptr<SceneObject> object)
{
    auto itr = m_treeWidgetItems.find(object->getID());
    if (itr == m_treeWidgetItems.end()) return;
    QTreeWidgetItem* treeItem = itr->second;
    selectObject(treeItem);
}

void MainWindow::onSelectedSceneObjectNameChangedSlot()
{
    /* Selected object name widget changed */
    QString newName = m_selectedObjectWidgetName->m_text->toPlainText();

    QTreeWidgetItem* selectedItem = m_sceneGraphWidget->currentItem();
    std::shared_ptr<SceneObject> object = selectedItem->data(0, Qt::UserRole).value<std::shared_ptr<SceneObject>>();

    object->m_name = newName.toStdString();
    selectedItem->setText(0, newName);
}

void MainWindow::onContextMenuSceneGraph(const QPoint& pos)
{
    QMenu contextMenu(tr("Context menu"), this);

    QAction action1("Add scene object", this);
    connect(&action1, SIGNAL(triggered()), this, SLOT(onAddSceneObjectSlot()));
    contextMenu.addAction(&action1);
    QAction action2("Remove scene object", this);
    connect(&action2, SIGNAL(triggered()), this, SLOT(onRemoveSceneObjectSlot()));
    contextMenu.addAction(&action2);
    QAction action3("Add point light", this);
    connect(&action3, SIGNAL(triggered()), this, SLOT(onAddPointLightSlot()));
    contextMenu.addAction(&action3);

    contextMenu.exec(m_sceneGraphWidget->mapToGlobal(pos));
}
