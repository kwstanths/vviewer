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
#include "WidgetMaterial.hpp"
#include "UIUtils.hpp"

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
    lightTransform.setRotationEuler(0, 0, 0);
    auto light = std::make_shared<DirectionalLight>(lightTransform, glm::vec3(1, 0.9, 0.8));
    m_vulkanWindow->m_scene->setDirectionalLight(light);
    
    m_widgetEnvironment = new WidgetEnvironment(nullptr, m_vulkanWindow->m_scene);

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

    QAction * onAddSceneObjectRootSlot = new QAction(tr("&Add a scene object at root"), this);
    onAddSceneObjectRootSlot->setStatusTip(tr("Add a scene object at root"));
    connect(onAddSceneObjectRootSlot, &QAction::triggered, this, &MainWindow::onAddSceneObjectRootSlot);
    QAction * m_actionCreateMaterial = new QAction(tr("&Add a material"), this);
    m_actionCreateMaterial->setStatusTip(tr("Add a material"));
    connect(m_actionCreateMaterial, &QAction::triggered, this, &MainWindow::onCreateMaterialSlot);

    QAction* m_actionRender = new QAction(tr("&Render scene (GPU) WIP"), this);
    m_actionRender->setStatusTip(tr("Render scene"));
    connect(m_actionRender, &QAction::triggered, this, &MainWindow::onRenderSceneSlot);
    /* work in progress, currently broken */
    m_actionRender->setEnabled(false);

    QAction* m_actionExport = new QAction(tr("&Export scene"), this);
    m_actionExport->setStatusTip(tr("EXport scene"));
    connect(m_actionExport, &QAction::triggered, this, &MainWindow::onExportSceneSlot);

    QMenu * m_menuImport = menuBar()->addMenu(tr("&Import"));
    m_menuImport->addAction(m_actionImportModel);
    m_menuImport->addAction(m_actionImportColorTexture);
    m_menuImport->addAction(m_actionImportOtherTexture);
    m_menuImport->addAction(m_actionImportHDRTexture);
    m_menuImport->addAction(m_actionImportEnvironmentMap);
    m_menuImport->addAction(m_actionImportMaterial);
    QMenu * m_menuAdd = menuBar()->addMenu(tr("&Add"));
    m_menuAdd->addAction(onAddSceneObjectRootSlot);
    m_menuAdd->addAction(m_actionCreateMaterial);
    QMenu* m_menuRender = menuBar()->addMenu(tr("&Render"));
    m_menuRender->addAction(m_actionRender);
    QMenu* m_menuExport = menuBar()->addMenu(tr("&Export"));
    m_menuExport->addAction(m_actionExport);
}

void MainWindow::selectObject(QTreeWidgetItem* selectedItem)
{
    /* If it's the previously selected item, return */
    if (selectedItem == m_selectedPreviousWidgetItem) return;
    m_sceneGraphWidget->blockSignals(true);

    /* When the selected object on the scene list changes, remove previous widgets from the controls, and add new */
    if (m_selectedObjectWidgetName != nullptr) {
        delete m_selectedObjectWidgetName;
        m_selectedObjectWidgetName = nullptr;
    }
    if (m_selectedObjectWidgetTransform != nullptr) {
        delete m_selectedObjectWidgetTransform;
        m_selectedObjectWidgetTransform = nullptr;
    }
    if (m_selectedObjectWidgetMaterial != nullptr) {
        delete m_selectedObjectWidgetMaterial;
        m_selectedObjectWidgetMaterial = nullptr;
    }

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

    if (sceneObject->getMesh() != nullptr) {
        m_selectedObjectWidgetMaterial = new WidgetMaterial(nullptr, sceneObject.get());
        m_layoutControls->addWidget(m_selectedObjectWidgetMaterial);
    }

    m_sceneGraphWidget->blockSignals(false);
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

    Material* material = m_vulkanWindow->m_renderer->createMaterial(materialName, MaterialType::MATERIAL_PBR_STANDARD);
    if (material == nullptr) return;

    Texture* albedo = m_vulkanWindow->m_renderer->createTexture(dirStd + "/albedo.png", VK_FORMAT_R8G8B8A8_SRGB);
    Texture* ao = m_vulkanWindow->m_renderer->createTexture(dirStd + "/ao.png", VK_FORMAT_R8G8B8A8_UNORM);
    Texture* metallic = m_vulkanWindow->m_renderer->createTexture(dirStd + "/metallic.png", VK_FORMAT_R8G8B8A8_UNORM);
    Texture* normal = m_vulkanWindow->m_renderer->createTexture(dirStd + "/normal.png", VK_FORMAT_R8G8B8A8_UNORM);
    Texture* roughness = m_vulkanWindow->m_renderer->createTexture(dirStd + "/roughness.png", VK_FORMAT_R8G8B8A8_UNORM);

    if (albedo != nullptr) static_cast<MaterialPBRStandard*>(material)->setAlbedoTexture(albedo);
    if (ao != nullptr) static_cast<MaterialPBRStandard*>(material)->setAOTexture(ao);
    if (metallic != nullptr)  static_cast<MaterialPBRStandard*>(material)->setMetallicTexture(metallic);
    if (normal != nullptr)  static_cast<MaterialPBRStandard*>(material)->setNormalTexture(normal);
    if (roughness != nullptr) static_cast<MaterialPBRStandard*>(material)->setRoughnessTexture(roughness);
}

void MainWindow::onAddSceneObjectRootSlot()
{
    DialogAddSceneObject * dialog = new DialogAddSceneObject(nullptr, "Add an object to the scene", getImportedModels(), getCreatedMaterials());
    dialog->exec();

    std::string selectedModel = dialog->getSelectedModel();
    if (selectedModel == "") return;

    std::shared_ptr<SceneObject> sceneObject = m_scene->addSceneObject(selectedModel, dialog->getTransform(), dialog->getSelectedMaterial());

    if (sceneObject == nullptr) utils::ConsoleWarning("Unable to add object to scene: " + selectedModel + ", with material: " + dialog->getSelectedMaterial());
    else {
        /* Set a name for the object */
        /* TODO set a some other way name */
        sceneObject->m_name = "New object (" + std::to_string(m_nObjects++) + ")";

        QTreeWidgetItem* parentItem = new QTreeWidgetItem({ QString(sceneObject->m_name.c_str()) });
        QVariant data;
        data.setValue(sceneObject);
        parentItem->setData(0, Qt::UserRole, data);
        m_sceneGraphWidget->addTopLevelItem(parentItem);
        m_treeWidgetItems.insert({ sceneObject->getID(), parentItem });

        for (auto& child : sceneObject->m_children) {
            QTreeWidgetItem* item = new QTreeWidgetItem({ QString(child->m_name.c_str()) });
            QVariant data;
            data.setValue(child);
            item->setData(0, Qt::UserRole, data);
            parentItem->addChild(item);
            m_treeWidgetItems.insert({ child->getID(), item });
        }
    }
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

    /* Get node selected from ui tree item */
    std::shared_ptr<SceneObject> selectedObject = selectedItem->data(0, Qt::UserRole).value<std::shared_ptr<SceneObject>>();

    /* Create an object on the scene graph as a child of the currently selected item */
    std::shared_ptr<SceneObject> sceneObject = m_scene->addSceneObject(selectedObject, selectedModel, dialog->getTransform(), dialog->getSelectedMaterial());

    if (sceneObject == nullptr) utils::ConsoleWarning("Unable to add object to scene: " + selectedModel + ", with material: " + dialog->getSelectedMaterial());
    else {
        /* Set a name for the object */
        /* TODO set the name some other way */
        sceneObject->m_name = "New object (" + std::to_string(m_nObjects++) + ")";

        QTreeWidgetItem* parentItem = new QTreeWidgetItem({ QString(sceneObject->m_name.c_str()) });
        QVariant data;
        data.setValue(sceneObject);
        parentItem->setData(0, Qt::UserRole, data);
        selectedItem->addChild(parentItem);
        m_treeWidgetItems.insert({ sceneObject->getID(), parentItem });

        for (auto& child : sceneObject->m_children) {
            QTreeWidgetItem* childItem = new QTreeWidgetItem({ QString(child->m_name.c_str()) });
            QVariant data;
            data.setValue(child);
            childItem->setData(0, Qt::UserRole, data);
            parentItem->addChild(childItem);
            m_treeWidgetItems.insert({ child->getID(), childItem });
        }
    }
}

void MainWindow::onRemoveSceneObjectSlot()
{
    /* TODO remove scene objects from m_treeWidgetItems */

    /* Get the currently selected tree item */
    QTreeWidgetItem* selectedItem = m_sceneGraphWidget->currentItem();
    std::shared_ptr<SceneObject> selectedObject = selectedItem->data(0, Qt::UserRole).value<std::shared_ptr<SceneObject>>();

    m_scene->removeSceneObject(selectedObject);
    
    /* Remove from UI */
    if (selectedObject->m_parent == nullptr) {
        m_sceneGraphWidget->removeItemWidget(selectedItem, 0);
        delete selectedItem;
    }
    else {
        selectedItem->parent()->removeChild(selectedItem);
        delete selectedItem;
    }
}

void MainWindow::onCreateMaterialSlot()
{
    DialogCreateMaterial * dialog = new DialogCreateMaterial(nullptr, "Create a material", getImportedModels());
    dialog->exec();

    std::string selectedModel = dialog->m_selectedName.toStdString();
    if (dialog->m_selectedMaterial == MaterialType::MATERIAL_NOT_SET) return;

    std::string materialName = dialog->m_selectedName.toStdString();
    if (materialName == "") {
        utils::ConsoleWarning("Material name can't be empty");
        return;
    }

    Material* material = m_vulkanWindow->m_renderer->createMaterial(materialName, dialog->m_selectedMaterial);
    if (material == nullptr) {
        utils::ConsoleWarning("Failed to create material");
    }
}

void MainWindow::onRenderSceneSlot()
{
    m_vulkanWindow->m_renderer->renderRT();
}

void MainWindow::onExportSceneSlot()
{
    m_scene->exportScene("testScene");
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

    contextMenu.exec(m_sceneGraphWidget->mapToGlobal(pos));
}
