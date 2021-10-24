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

MainWindow::MainWindow(QWidget *parent) :QMainWindow(parent) {
    
    QWidget * widgetControls = initControlsWidget();
    QWidget * widgetVulkan = initVulkanWindowWidget();

    QHBoxLayout * layout_main = new QHBoxLayout();
    layout_main->addWidget(initLeftPanel());
    layout_main->addWidget(widgetVulkan);
    layout_main->addWidget(widgetControls);

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
    return widget_controls;
}

void MainWindow::createMenu()
{
    m_actionImport = new QAction(tr("&Import a model"), this);
    m_actionImport->setStatusTip(tr("Import a model"));
    connect(m_actionImport, &QAction::triggered, this, &MainWindow::onImportModelSlot);

    m_actionAddSceneObject = new QAction(tr("&Add a scene object"), this);
    m_actionAddSceneObject->setStatusTip(tr("Add a scene object"));
    connect(m_actionAddSceneObject, &QAction::triggered, this, &MainWindow::onAddSceneObjectSlot);

    m_actionCreateMaterial = new QAction(tr("&Create a material"), this);
    m_actionCreateMaterial->setStatusTip(tr("Create a material"));
    connect(m_actionCreateMaterial, &QAction::triggered, this, &MainWindow::onCreateMaterialSlot);

    m_menuFile = menuBar()->addMenu(tr("&File"));
    m_menuFile->addAction(m_actionImport);
    m_menuFile->addAction(m_actionAddSceneObject);
    m_menuFile->addAction(m_actionCreateMaterial);
}

QStringList MainWindow::getImportedModels()
{
    QStringList importedModels;
    AssetManager<std::string, MeshModel *>& instance = AssetManager<std::string, MeshModel *>::getInstance();
    for (auto itr = instance.begin(); itr != instance.end(); ++itr) {
        importedModels.push_back(QString::fromStdString(itr->first));
    }
    return importedModels;
}

QStringList MainWindow::getCreatedMaterials()
{
    QStringList createdMaterials;
    AssetManager<std::string, Material *>& instance = AssetManager<std::string, Material *>::getInstance();
    for (auto itr = instance.begin(); itr != instance.end(); ++itr) {
        createdMaterials.push_back(QString::fromStdString(itr->first));
    }
    return createdMaterials;
}

void MainWindow::onImportModelSlot()
{   
    QString filename = QFileDialog::getOpenFileName(this,
        tr("Import model"), "",
        tr("Model (*.obj);;All Files (*)"));

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
    m_selectedObjectWidgetMaterial = new WidgetMaterialPBR(nullptr, object, static_cast<MaterialPBR *>(object->getMaterial()), getCreatedMaterials());

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
