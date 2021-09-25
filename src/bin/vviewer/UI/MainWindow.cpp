#include "MainWindow.hpp"

#include <qvulkaninstance.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qmenubar.h>
#include <qfiledialog.h>
#include <QVariant>

#include <utils/Console.hpp>

#include "DialogAddSceneObject.hpp"

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

    widget_controls->setFixedWidth(250);
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

    m_menuFile = menuBar()->addMenu(tr("&File"));
    m_menuFile->addAction(m_actionImport);
    m_menuFile->addAction(m_actionAddSceneObject);
}

void MainWindow::onImportModelSlot()
{   
    QString filename = QFileDialog::getOpenFileName(this,
        tr("Import model"), "",
        tr("Model (*.obj);;All Files (*)"));

    bool ret = m_vulkanWindow->ImportMeshModel(filename.toStdString());
 
    if (ret) {
        utils::ConsoleInfo("Model imported");
        m_importedModels.append(filename);
    }
}

void MainWindow::onAddSceneObjectSlot()
{
    DialogAddSceneObject * dialog = new DialogAddSceneObject(nullptr, "Add an object to the scene", m_importedModels);
    dialog->exec();

    std::string selectedModel = dialog->getSelectedModel();
    if (selectedModel == "") return;

    SceneObject * object = m_vulkanWindow->AddSceneObject(selectedModel, dialog->getTransform());
    
    /* TODO set a some other way name */
    object->m_name = "New object (" + std::to_string(m_nObjects++) + ")";

    if (object == nullptr) utils::ConsoleWarning("Model not present");
    else {
        QListWidgetItem * item = new QListWidgetItem(QString(object->m_name.c_str()));
        QVariant data;
        data.setValue(object);
        item->setData(Qt::UserRole, data);
        m_sceneObjects->addItem(item);
    }
}

void MainWindow::onSelectedSceneObjectChangedSlot()
{
    if (m_selectedObjectWidgetName != nullptr) {
        delete m_selectedObjectWidgetName;
        delete m_selectedObjectWidgetTransform;
    }

    QListWidgetItem * selectedItem = m_sceneObjects->currentItem();
    SceneObject * object = selectedItem->data(Qt::UserRole).value<SceneObject *>();
    
    m_selectedObjectWidgetName = new WidgetName(nullptr, QString(object->m_name.c_str()));
    connect(m_selectedObjectWidgetName->m_text, &QTextEdit::textChanged, this, &MainWindow::onSelectedSceneObjectNameChangedSlot);

    m_selectedObjectWidgetTransform = new WidgetTransform(nullptr);
    m_selectedObjectWidgetTransform->setTransform(object->getTransform());
    connect(m_selectedObjectWidgetTransform->m_positionX, SIGNAL(valueChanged(double)), this, SLOT(onSelectedSceneObjectTransformChangedSlot(double)));
    connect(m_selectedObjectWidgetTransform->m_positionY, SIGNAL(valueChanged(double)), this, SLOT(onSelectedSceneObjectTransformChangedSlot(double)));
    connect(m_selectedObjectWidgetTransform->m_positionZ, SIGNAL(valueChanged(double)), this, SLOT(onSelectedSceneObjectTransformChangedSlot(double)));
    connect(m_selectedObjectWidgetTransform->m_scaleX, SIGNAL(valueChanged(double)), this, SLOT(onSelectedSceneObjectTransformChangedSlot(double)));
    connect(m_selectedObjectWidgetTransform->m_scaleY, SIGNAL(valueChanged(double)), this, SLOT(onSelectedSceneObjectTransformChangedSlot(double)));
    connect(m_selectedObjectWidgetTransform->m_scaleZ, SIGNAL(valueChanged(double)), this, SLOT(onSelectedSceneObjectTransformChangedSlot(double)));
    connect(m_selectedObjectWidgetTransform->m_rotationX, SIGNAL(valueChanged(double)), this, SLOT(onSelectedSceneObjectTransformChangedSlot(double)));
    connect(m_selectedObjectWidgetTransform->m_rotationY, SIGNAL(valueChanged(double)), this, SLOT(onSelectedSceneObjectTransformChangedSlot(double)));
    connect(m_selectedObjectWidgetTransform->m_rotationZ, SIGNAL(valueChanged(double)), this, SLOT(onSelectedSceneObjectTransformChangedSlot(double)));

    m_layoutControls->addWidget(m_selectedObjectWidgetName);
    m_layoutControls->addWidget(m_selectedObjectWidgetTransform);
}

void MainWindow::onSelectedSceneObjectNameChangedSlot()
{
    QListWidgetItem * selectedItem = m_sceneObjects->currentItem();

    QString newName = m_selectedObjectWidgetName->m_text->toPlainText();
    selectedItem->setText(newName);

    SceneObject * object = selectedItem->data(Qt::UserRole).value<SceneObject *>();
    object->m_name = newName.toStdString();
}

void MainWindow::onSelectedSceneObjectTransformChangedSlot(double d)
{
    QListWidgetItem * selectedItem = m_sceneObjects->currentItem();

    Transform newTransform = m_selectedObjectWidgetTransform->getTransform();

    SceneObject * object = selectedItem->data(Qt::UserRole).value<SceneObject *>();
    object->setTransform(newTransform);
}

