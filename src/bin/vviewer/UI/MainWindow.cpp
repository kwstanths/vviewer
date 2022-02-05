#include "MainWindow.hpp"

#include <qvulkaninstance.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qmenubar.h>
#include <qfiledialog.h>
#include <QVariant>

#include <utils/Console.hpp>

#include "DialogAddSceneObject.hpp"
#include "DialogCreateMaterial.hpp"
#include "UIUtils.hpp"

MainWindow::MainWindow(QWidget *parent) :QMainWindow(parent) {
    
    QWidget * widgetVulkan = initVulkanWindowWidget();

    QHBoxLayout * layout_main = new QHBoxLayout();
    layout_main->addWidget(initLeftPanel());
    layout_main->addWidget(widgetVulkan);
    layout_main->addWidget(initControlsWidget());


    QWidget * widget_main = new QWidget();
    widget_main->setLayout(layout_main);

    createMenu();

    setCentralWidget(widget_main);
    resize(1600, 1000);
}

MainWindow::~MainWindow() {

}

QWidget * MainWindow::initLeftPanel()
{
    m_sceneObjects = new QListWidget();
    m_sceneObjects->setStyleSheet("background-color: rgba(240, 240, 240, 255);");
    connect(m_sceneObjects, &QListWidget::itemSelectionChanged, this, &MainWindow::onSelectedSceneObjectChangedSlot);

    QVBoxLayout * layout_main = new QVBoxLayout();
    layout_main->addWidget(new QLabel("Scene:"));
    layout_main->addWidget(m_sceneObjects); 
    
    QWidget * widget_main = new QWidget();
    widget_main->setLayout(layout_main);

    widget_main->setFixedWidth(200);
    return widget_main;
}

QWidget * MainWindow::initVulkanWindowWidget()
{
    const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
    };

    QByteArrayList layers;
    QByteArrayList extensions;
#ifdef NDEBUG

#else
    //layers.append("VK_LAYER_GOOGLE_threading");
    //layers.append("VK_LAYER_LUNARG_parameter_validation");
    //layers.append("VK_LAYER_LUNARG_object_tracker");
    //layers.append("VK_LAYER_LUNARG_core_validation");
    //layers.append("VK_LAYER_LUNARG_image");
    //layers.append("VK_LAYER_LUNARG_swapchain");
    //layers.append("VK_LAYER_GOOGLE_unique_objects");
    layers.append("VK_LAYER_KHRONOS_validation");

    extensions.append("VK_EXT_debug_utils");
#endif

    m_vulkanInstance = new QVulkanInstance();
    /* Enable validation layers, if available */
    m_vulkanInstance->setLayers(layers);
    m_vulkanInstance->setExtensions(extensions);

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
    lightTransform.setRotationEuler(0, 0, 0);
    auto light = std::make_shared<DirectionalLight>(lightTransform, glm::vec3(1, 0.9, 0.8));
    m_vulkanWindow->m_directionalLight = light;
    m_widgetEnvironment = new WidgetEnvironment(nullptr, light);

    QTabWidget* widget_tab = new QTabWidget();
    widget_tab->insertTab(0, widget_controls, "Scene object");
    widget_tab->insertTab(1, m_widgetEnvironment, "Environment");

    widget_tab->setFixedWidth(280);
    return widget_tab;
}

void MainWindow::createMenu()
{
    QAction * m_actionImportModel = new QAction(tr("&Import a model"), this);
    m_actionImportModel->setStatusTip(tr("Import a model"));
    connect(m_actionImportModel, &QAction::triggered, this, &MainWindow::onImportModelSlot);
    QAction * m_actionImportColorTexture = new QAction(tr("&Import color textures"), this);
    m_actionImportColorTexture->setStatusTip(tr("Import color textures"));
    connect(m_actionImportColorTexture, &QAction::triggered, this, &MainWindow::onImportTextureColorSlot);
    QAction * m_actionImportOtherTexture = new QAction(tr("&Import other textures"), this);
    m_actionImportOtherTexture->setStatusTip(tr("Import other textures"));
    connect(m_actionImportOtherTexture, &QAction::triggered, this, &MainWindow::onImportTextureOtherSlot);
    QAction * m_actionImportHDRTexture = new QAction(tr("&Import HDR texture"), this);
    m_actionImportHDRTexture->setStatusTip(tr("Import HDR texture"));
    connect(m_actionImportHDRTexture, &QAction::triggered, this, &MainWindow::onImportTextureHDRSlot);
    QAction* m_actionImportEnvironmentMap = new QAction(tr("&Import environment map"), this);
    m_actionImportEnvironmentMap->setStatusTip(tr("Import environment map"));
    connect(m_actionImportEnvironmentMap, &QAction::triggered, this, &MainWindow::onImportEnvironmentMap);
    QAction* m_actionImportMaterial = new QAction(tr("&Import material"), this);
    m_actionImportMaterial->setStatusTip(tr("Import material"));
    connect(m_actionImportMaterial, &QAction::triggered, this, &MainWindow::onImportMaterial);

    QAction * m_actionAddSceneObject = new QAction(tr("&Add a scene object"), this);
    m_actionAddSceneObject->setStatusTip(tr("Add a scene object"));
    connect(m_actionAddSceneObject, &QAction::triggered, this, &MainWindow::onAddSceneObjectSlot);
    QAction * m_actionCreateMaterial = new QAction(tr("&Add a material"), this);
    m_actionCreateMaterial->setStatusTip(tr("Add a material"));
    connect(m_actionCreateMaterial, &QAction::triggered, this, &MainWindow::onCreateMaterialSlot);

    QMenu * m_menuImport = menuBar()->addMenu(tr("&Import"));
    m_menuImport->addAction(m_actionImportModel);
    m_menuImport->addAction(m_actionImportColorTexture);
    m_menuImport->addAction(m_actionImportOtherTexture);
    m_menuImport->addAction(m_actionImportHDRTexture);
    m_menuImport->addAction(m_actionImportEnvironmentMap);
    m_menuImport->addAction(m_actionImportMaterial);
    QMenu * m_menuAdd = menuBar()->addMenu(tr("&Add"));
    m_menuAdd->addAction(m_actionAddSceneObject);
    m_menuAdd->addAction(m_actionCreateMaterial);
}

void MainWindow::onImportModelSlot()
{   
    QString filename = QFileDialog::getOpenFileName(this,
        tr("Import model"), "./assets/models",
        tr("Model (*.obj);;All Files (*)"));

    if (filename == "") return;

    bool ret = m_vulkanWindow->m_renderer->createVulkanMeshModel(filename.toStdString());
 
    if (ret) {
        utils::ConsoleInfo("Model imported");
        if (m_selectedObjectWidgetMeshModel != nullptr) {
            m_selectedObjectWidgetMeshModel->m_models->blockSignals(true);
            m_selectedObjectWidgetMeshModel->m_models->clear();
            m_selectedObjectWidgetMeshModel->m_models->addItems(getImportedModels());
            m_selectedObjectWidgetMeshModel->m_models->blockSignals(false);
        }
    }
}

void MainWindow::onImportTextureColorSlot()
{
    QStringList filenames = QFileDialog::getOpenFileNames(this,
        tr("Import textures"), "./assets",
        tr("Textures (*.png);;All Files (*)"));

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
        tr("Textures (*.png);;All Files (*)"));

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

    Material* material = m_vulkanWindow->m_renderer->createMaterial(materialName, glm::vec4(1, 1, 1, 1), 1, 1, 1, 0);
    if (material == nullptr) return;

    Texture* albedo = m_vulkanWindow->m_renderer->createTexture(dirStd + "/albedo.png", VK_FORMAT_R8G8B8A8_SRGB);
    Texture* ao = m_vulkanWindow->m_renderer->createTexture(dirStd + "/ao.png", VK_FORMAT_R8G8B8A8_UNORM);
    Texture* metallic = m_vulkanWindow->m_renderer->createTexture(dirStd + "/metallic.png", VK_FORMAT_R8G8B8A8_UNORM);
    Texture* normal = m_vulkanWindow->m_renderer->createTexture(dirStd + "/normal.png", VK_FORMAT_R8G8B8A8_UNORM);
    Texture* roughness = m_vulkanWindow->m_renderer->createTexture(dirStd + "/roughness.png", VK_FORMAT_R8G8B8A8_UNORM);

    if (albedo != nullptr) static_cast<MaterialPBR*>(material)->setAlbedoTexture(albedo);
    if (ao != nullptr) static_cast<MaterialPBR*>(material)->setAOTexture(ao);
    if (metallic != nullptr)  static_cast<MaterialPBR*>(material)->setMetallicTexture(metallic);
    if (normal != nullptr)  static_cast<MaterialPBR*>(material)->setNormalTexture(normal);
    if (roughness != nullptr) static_cast<MaterialPBR*>(material)->setRoughnessTexture(roughness);
}

void MainWindow::onAddSceneObjectSlot()
{
    DialogAddSceneObject * dialog = new DialogAddSceneObject(nullptr, "Add an object to the scene", getImportedModels(), getCreatedMaterials());
    dialog->exec();

    std::string selectedModel = dialog->getSelectedModel();
    if (selectedModel == "") return;

    SceneObject * object = m_vulkanWindow->m_renderer->addSceneObject(selectedModel, dialog->getTransform(), dialog->getSelectedMaterial());

    if (object == nullptr) utils::ConsoleWarning("Unable to add object to scene: " + selectedModel + ", with material: " + dialog->getSelectedMaterial());
    else {
        /* Set a name for the object */
        /* TODO set a some other way name */
        object->m_name = "New object (" + std::to_string(m_nObjects++) + ")";

        /* Add it on the UI list of objects */
        QListWidgetItem * item = new QListWidgetItem(QString(object->m_name.c_str()));
        QVariant data;
        data.setValue(object);
        /* Connect the UI entry with the SceneObject item */
        item->setData(Qt::UserRole, data);
        m_sceneObjects->addItem(item);
    }
}

void MainWindow::onCreateMaterialSlot()
{
    DialogCreateMaterial * dialog = new DialogCreateMaterial(nullptr, "Create a material", getImportedModels());
    dialog->exec();

    std::string selectedModel = dialog->m_selectedName.toStdString();
    if (dialog->m_selectedMaterial == MaterialType::MATERIAL_NOT_SET) return;

    Material * material = m_vulkanWindow->m_renderer->createMaterial(dialog->m_selectedName.toStdString(), glm::vec4(0.5, 0.5, 0.5, 1), 0.5, 0.5, 1, 0);
    if (material == nullptr) {
        utils::ConsoleWarning("Failed to create material");
    }
}

void MainWindow::onSelectedSceneObjectChangedSlot()
{
    /* When the selected object on the scene list changes, remove previous widgets from the controls, and new */
    if (m_selectedObjectWidgetName != nullptr) {
        delete m_selectedObjectWidgetName;
        delete m_selectedObjectWidgetTransform;
        delete m_selectedObjectWidgetMeshModel;
        delete m_selectedObjectWidgetMaterial;
        m_selectedObjectWidgetName = nullptr;
        m_selectedObjectWidgetTransform = nullptr;
        m_selectedObjectWidgetMeshModel = nullptr;
        m_selectedObjectWidgetMaterial = nullptr;
    }

    /* Get currently selected object, and the corresponding SceneObject */
    QListWidgetItem * selectedItem = m_sceneObjects->currentItem();
    SceneObject * object = selectedItem->data(Qt::UserRole).value<SceneObject *>();
    
    /* Create UI elements for its components, connect them to slots, and add them to the controls widget */
    m_selectedObjectWidgetName = new WidgetName(nullptr, QString(object->m_name.c_str()));
    connect(m_selectedObjectWidgetName->m_text, &QTextEdit::textChanged, this, &MainWindow::onSelectedSceneObjectNameChangedSlot);

    m_selectedObjectWidgetTransform = new WidgetTransform(nullptr, object);
    m_selectedObjectWidgetMeshModel = new WidgetMeshModel(nullptr, object, getImportedModels());
    m_selectedObjectWidgetMaterial = new WidgetMaterialPBR(nullptr, object, static_cast<MaterialPBR *>(object->getMaterial()));

    m_layoutControls->addWidget(m_selectedObjectWidgetName);
    m_layoutControls->addWidget(m_selectedObjectWidgetTransform);
    m_layoutControls->addWidget(m_selectedObjectWidgetMeshModel);
    m_layoutControls->addWidget(m_selectedObjectWidgetMaterial);
}

void MainWindow::onSelectedSceneObjectNameChangedSlot()
{
    /* Selected object name widget changed */
    QString newName = m_selectedObjectWidgetName->m_text->toPlainText();

    QListWidgetItem * selectedItem = m_sceneObjects->currentItem();

    SceneObject * object = selectedItem->data(Qt::UserRole).value<SceneObject *>();
    object->m_name = newName.toStdString();
    selectedItem->setText(newName);
}
