#include "MainWindow.hpp"

#include <qvulkaninstance.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qmenubar.h>
#include <qfiledialog.h>
#include <qvariant.h>
#include <qmessagebox.h>

#include <glm/glm.hpp>

#include <string>
#include <utils/Console.hpp>

#include "DialogAddSceneObject.hpp"
#include "DialogCreateMaterial.hpp"
#include "DialogSceneExport.hpp"
#include "DialogSceneRender.hpp"
#include "WidgetMaterial.hpp"
#include "UIUtils.hpp"

#include "core/AssetManager.hpp"
#include "core/Import.hpp"
#include "vulkan/renderers/VulkanRendererRayTracing.hpp"

MainWindow::MainWindow(QWidget *parent) :QMainWindow(parent) {
    
    QWidget * widgetVulkan = initVulkanWindowWidget();

    QHBoxLayout * layout_main = new QHBoxLayout();
    layout_main->addWidget(initLeftPanel());
    layout_main->addWidget(widgetVulkan);
    layout_main->addWidget(initControlsWidget());

    QWidget * widget_main = new QWidget();
    widget_main->setLayout(layout_main);

    m_scene = m_vulkanWindow->getScene();

    createMenu();

    setCentralWidget(widget_main);
    resize(1600, 1100);
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
    m_vulkanWindow = new VulkanWindow();
    connect(m_vulkanWindow, &VulkanWindow::sceneObjectSelected, this, &MainWindow::onSelectedSceneObjectChangedSlot3DScene);
    connect(m_vulkanWindow, &VulkanWindow::initializationFinished, this, &MainWindow::onStartUpInitialization);

    return QWidget::createWindowContainer(m_vulkanWindow);
}

QWidget * MainWindow::initControlsWidget()
{
    m_layoutControls = new QVBoxLayout();
    m_layoutControls->setAlignment(Qt::AlignTop);
    QWidget * widget_controls = new QWidget();
    widget_controls->setLayout(m_layoutControls);
    
    m_widgetEnvironment = new WidgetEnvironment(nullptr, m_vulkanWindow->getScene());

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
    QAction* actionImportMaterialZipStack = new QAction(tr("&Import material ZIP"), this);
    actionImportMaterialZipStack->setStatusTip(tr("Import material ZIP"));
    connect(actionImportMaterialZipStack, &QAction::triggered, this, &MainWindow::onImportMaterialZipStackSlot);
    QAction* actionImportScene = new QAction(tr("&Import scene"), this);
    actionImportScene->setStatusTip(tr("Import scene"));
    connect(actionImportScene, &QAction::triggered, this, &MainWindow::onImportScene);

    QAction * addSceneObjectRootSlot = new QAction(tr("&Add a scene object at root"), this);
    addSceneObjectRootSlot->setStatusTip(tr("Add a scene object at root"));
    connect(addSceneObjectRootSlot, &QAction::triggered, this, &MainWindow::onAddSceneObjectRootSlot);
    QAction * actionCreateMaterial = new QAction(tr("&Add a material"), this);
    actionCreateMaterial->setStatusTip(tr("Add a material"));
    connect(actionCreateMaterial, &QAction::triggered, this, &MainWindow::onAddMaterialSlot);
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
    m_menuImport->addAction(actionImportMaterialZipStack);
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

std::shared_ptr<SceneObject> MainWindow::getSceneObject(QTreeWidgetItem* item) const
{
    return item->data(0, Qt::UserRole).value<std::shared_ptr<SceneObject>>();
}

std::pair<QTreeWidgetItem*, std::shared_ptr<SceneObject>> MainWindow::createEmptySceneObject(std::string name, const Transform& transform, QTreeWidgetItem* parent)
{
    /* If parent is null, add at root */
    if (parent == nullptr)
    {
        std::shared_ptr<SceneObject> newObject = m_scene->addSceneObject(name, transform);
        QTreeWidgetItem* widgetItem = createTreeWidgetItem(newObject);

        m_sceneGraphWidget->addTopLevelItem(widgetItem); 

        return {widgetItem, newObject};
    } else {
        std::shared_ptr<SceneObject> parentObject = getSceneObject(parent);

        std::shared_ptr<SceneObject> newObject = m_scene->addSceneObject(name, parentObject, transform);
        QTreeWidgetItem* widgetItem = createTreeWidgetItem(newObject);

        parent->addChild(widgetItem);

        return {widgetItem, newObject};
    }
}

void MainWindow::selectObject(QTreeWidgetItem* selectedItem)
{
    /* If it's the previously selected item, return */
    if (selectedItem == m_selectedPreviousWidgetItem) return;
    m_sceneGraphWidget->blockSignals(true);

    /* When the selected object on the scene list changes, remove previous widgets from the controls, and add new */
    clearControlsUI();

    /* Get selected object */
    std::shared_ptr<SceneObject> sceneObject = getSceneObject(selectedItem);
    /* Remove UI selection if needed */
    if (!selectedItem->isSelected()) {
        selectedItem->setSelected(true);
        selectedItem->setExpanded(true);
    }

    /* Remove selection from previously selected item */
    if (m_selectedPreviousWidgetItem != nullptr) {
        m_selectedPreviousWidgetItem->setSelected(false);
        std::shared_ptr<SceneObject> previousSelected = getSceneObject(m_selectedPreviousWidgetItem);
        previousSelected->m_isSelected = false;
    }

    /* Set selection to new item */
    m_selectedPreviousWidgetItem = selectedItem;
    sceneObject->m_isSelected = true;
    m_vulkanWindow->getRenderer()->setSelectedObject(sceneObject);

    /* Create UI elements for its components, connect them to slots, and add them to the controls widget */
    m_selectedObjectWidgetName = new WidgetName(nullptr, QString(sceneObject->m_name.c_str()));
    connect(m_selectedObjectWidgetName->m_text, &QTextEdit::textChanged, this, &MainWindow::onSelectedSceneObjectNameChangedSlot);

    m_selectedObjectWidgetTransform = new WidgetTransform(nullptr, sceneObject);
    connect(m_vulkanWindow, &VulkanWindow::selectedObjectPositionChanged, m_selectedObjectWidgetTransform, &WidgetTransform::onPositionChangedFrom3DScene);

    m_layoutControls->addWidget(m_selectedObjectWidgetName);
    m_layoutControls->addWidget(m_selectedObjectWidgetTransform);

    if (sceneObject->has<ComponentMesh>())
    {
        m_selectedObjectWidgetMeshModel = new WidgetMeshModel(nullptr, sceneObject->get<ComponentMesh>());
        m_layoutControls->addWidget(m_selectedObjectWidgetMeshModel);
    }

    if (sceneObject->has<ComponentMaterial>()) 
    {
        m_selectedObjectWidgetMaterial = new WidgetMaterial(nullptr, sceneObject->get<ComponentMaterial>());
        m_layoutControls->addWidget(m_selectedObjectWidgetMaterial);
    }

    if (sceneObject->has<ComponentPointLight>()) 
    {
        m_selectedObjectWidgetPointLight = new WidgetPointLight(nullptr, sceneObject->get<ComponentPointLight>());
        m_layoutControls->addWidget(m_selectedObjectWidgetPointLight);
    }

    m_sceneGraphWidget->blockSignals(false);
}

void MainWindow::removeObjectFromScene(QTreeWidgetItem* treeItem)
{
    /* Get corresponding scene object */
    std::shared_ptr<SceneObject> selectedObject = getSceneObject(treeItem);
    /* Remove scene object from scene */
    m_scene->removeSceneObject(selectedObject);

    /* Set selected to null */
    m_vulkanWindow->getRenderer()->setSelectedObject(nullptr);
    m_selectedPreviousWidgetItem = nullptr;

    /* Remove from m_treeWidgetItems recursively */
    std::function<void(QTreeWidgetItem*)> removeChild;
    removeChild = [&](QTreeWidgetItem* child) { 
        std::shared_ptr<SceneObject> object = getSceneObject(child);
        m_treeWidgetItems.erase(object->getID());
        for(size_t i=0; i < child->childCount(); i++) {
            removeChild(child->child(i));
        }    
    };
    m_treeWidgetItems.erase(selectedObject->getID());
    for(size_t i=0; i < treeItem->childCount(); i++) {
        removeChild(treeItem->child(i));
    }

    /* Delete and remove from UI */
    delete treeItem;
}

void MainWindow::addSceneObjectMeshes(QTreeWidgetItem * parentItem, std::string modelName, std::string material)
{
    AssetManager<std::string, MeshModel>& instanceModels = AssetManager<std::string, MeshModel>::getInstance();
    if (!instanceModels.isPresent(modelName))
    {
        utils::ConsoleWarning("VulkanScene::addSceneObjectMeshes(): Mesh model " + modelName + " is not imported");
        return;
    }
    AssetManager<std::string, Material>& instanceMaterials = AssetManager<std::string, Material>::getInstance();
    if (!instanceMaterials.isPresent(material)) 
    {
        utils::ConsoleWarning("VulkanScene::addSceneObjectMeshes(): Material " + material + " is not created");
        return;
    }

    auto meshModel = instanceModels.get(modelName);
    std::vector<std::shared_ptr<Mesh>> modelMeshes = meshModel->getMeshes();
    
    std::shared_ptr<SceneObject> parentObject = getSceneObject(parentItem);

    /* For all meshes inside the mesh model, add them in the scene and the UI */
    for (auto& m : modelMeshes) {
        auto child = createEmptySceneObject(m->m_name, Transform(), parentItem);

        auto& cm = ComponentManager::getInstance();
        child.second->assign(cm.create<ComponentMesh>(m));
        child.second->assign(cm.create<ComponentMaterial>(instanceMaterials.get(material)));
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

bool MainWindow::addImportedSceneObject(const ImportedSceneObject& object, 
            const std::unordered_map<std::string, ImportedSceneMaterial>& materials,
            const std::unordered_map<std::string, ImportedSceneLightMaterial>& lights,
            QTreeWidgetItem * parentItem, std::string sceneFolder)
{
    AssetManager<std::string, Material>& instanceMaterials = AssetManager<std::string, Material>::getInstance();
    AssetManager<std::string, LightMaterial>& instanceLightMaterials = AssetManager<std::string, LightMaterial>::getInstance();

    /* Check if it's a directional light node */
    if (object.light.has_value() && object.light->type == ImportedSceneLightType::DISTANT) 
    {
        /* Check if the light material has already been created */
        std::shared_ptr<LightMaterial> lightMaterial;
        if (!instanceLightMaterials.isPresent(object.light->lightMaterial))
        {
            auto importedLightMaterialItr = lights.find(object.light->lightMaterial);
            if (importedLightMaterialItr == lights.end())
            {
                throw std::runtime_error("addImportedSceneObject(): Referenced light material in the scene hasn't been imported properly");
            }
            lightMaterial = std::make_shared<LightMaterial>(importedLightMaterialItr->second.name, importedLightMaterialItr->second.color, importedLightMaterialItr->second.intensity);
            instanceLightMaterials.add(object.light->lightMaterial, lightMaterial);
        } else 
        {
            lightMaterial = instanceLightMaterials.get(object.light->lightMaterial);
        }

        auto light = std::make_shared<DirectionalLight>(object.transform, lightMaterial);
    
        m_scene->setDirectionalLight(light);
        m_widgetEnvironment->setDirectionalLight(light);
        return true;
    }

    /* Create new scene object */
    auto newSceneObject = createEmptySceneObject(object.name, object.transform, parentItem);

    if (object.mesh.has_value() && object.light.has_value() && object.light->type == ImportedSceneLightType::MESH)
    {
        throw std::runtime_error("addImportedSceneObject(): Imported scene node has both a mesh component and a light mesh component");
    }

    /* Add mesh component */
    if (object.mesh.has_value()) 
    {
        /* Import mesh material if needed */
        if (!instanceMaterials.isPresent(object.mesh->material))
        {
            auto importedSceneMaterialItr = materials.find(object.mesh->material);
            if (importedSceneMaterialItr == materials.end())
            {
                throw std::runtime_error("addImportedSceneObject(): Referenced material in the scene hasn't been imported properly");
            }

            const ImportedSceneMaterial& m = importedSceneMaterialItr->second;
            if (m.type == ImportedSceneMaterialType::STACK)
            {
                /* Check if it's a zip file, or a directory of textures */
                if (m.stackDir != "") m_vulkanWindow->importMaterial(m.name, sceneFolder + m.stackDir);
                else if (m.stackFile != "") m_vulkanWindow->importZipMaterial(m.name, sceneFolder + m.stackFile);
            }
            else if (m.type == ImportedSceneMaterialType::DIFFUSE)
            {
                auto mat = std::dynamic_pointer_cast<MaterialLambert>(m_vulkanWindow->getRenderer()->createMaterial(m.name, MaterialType::MATERIAL_LAMBERT));

                mat->albedo() = glm::vec4(m.albedo, 1);
                if (m.albedoTexture != "") {
                    auto tex = m_vulkanWindow->getRenderer()->createTexture(sceneFolder + m.albedoTexture, VK_FORMAT_R8G8B8A8_SRGB);
                    mat->setAlbedoTexture(tex);
                }
                if (m.normalTexture != "") {
                    auto tex = m_vulkanWindow->getRenderer()->createTexture(sceneFolder + m.normalTexture, VK_FORMAT_R8G8B8A8_UNORM);
                    mat->setNormalTexture(tex);
                }

                mat->uTiling() = m.scale.x;
                mat->vTiling() = m.scale.y;
            } 
            else if (m.type == ImportedSceneMaterialType::DISNEY)
            {
                auto mat = std::dynamic_pointer_cast<MaterialPBRStandard>(m_vulkanWindow->getRenderer()->createMaterial(m.name, MaterialType::MATERIAL_PBR_STANDARD));

                mat->albedo() = glm::vec4(m.albedo, 1);
                if (m.albedoTexture != "") {
                    auto tex = m_vulkanWindow->getRenderer()->createTexture(sceneFolder + m.albedoTexture, VK_FORMAT_R8G8B8A8_SRGB);
                    if (tex != nullptr) mat->setAlbedoTexture(tex);
                }
                mat->roughness() = m.roughness;
                if (m.roughnessTexture != "") {
                    auto tex = m_vulkanWindow->getRenderer()->createTexture(sceneFolder + m.roughnessTexture, VK_FORMAT_R8G8B8A8_UNORM);
                    if (tex != nullptr) mat->setRoughnessTexture(tex);
                }
                mat->metallic() = m.metallic;
                if (m.metallicTexture != "") {
                    auto tex = m_vulkanWindow->getRenderer()->createTexture(sceneFolder + m.metallicTexture, VK_FORMAT_R8G8B8A8_UNORM);
                    if (tex != nullptr) mat->setMetallicTexture(tex);
                }
                if (m.normalTexture != "") {
                    auto tex = m_vulkanWindow->getRenderer()->createTexture(sceneFolder + m.normalTexture, VK_FORMAT_R8G8B8A8_UNORM);
                    if (tex != nullptr) mat->setNormalTexture(tex);
                }
                
                mat->uTiling() = m.scale.x;
                mat->vTiling() = m.scale.y;
            }
        }

        auto meshModel = m_vulkanWindow->getRenderer()->createVulkanMeshModel(sceneFolder + object.mesh->path);
        if (meshModel != nullptr) {
            if (object.mesh->submesh == -1 || object.mesh->submesh >= meshModel->getMeshes().size()) {
                /* Imported mesh defines a path, but not a submesh. Create a new node and add all the meshes as children */
                auto child = createEmptySceneObject("meshModel", Transform(), newSceneObject.first);
                
                addSceneObjectMeshes(child.first, sceneFolder + object.mesh->path, object.mesh->material);
            } else{
                /* Else, assign the corresponding mesh to the object */
                auto& cm = ComponentManager::getInstance();

                auto mesh = meshModel->getMeshes()[object.mesh->submesh];
                newSceneObject.second->assign(cm.create<ComponentMesh>(mesh));
                auto mat = instanceMaterials.get(object.mesh->material);
                newSceneObject.second->assign(cm.create<ComponentMaterial>(mat));
            }
        } else {
            utils::ConsoleWarning("addImportedSceneObject(): Failed to import: " + sceneFolder + object.mesh->path);
        }
    }
    
    /* Add light component */
    if (object.light.has_value()) 
    {
        if (object.light->type == ImportedSceneLightType::POINT) 
        {
            /* Check if the light material has already been created */
            std::shared_ptr<LightMaterial> lightMaterial;
            if (!instanceLightMaterials.isPresent(object.light->lightMaterial))
            {
                auto importedLightMaterialItr = lights.find(object.light->lightMaterial);
                if (importedLightMaterialItr == lights.end())
                {
                    throw std::runtime_error("addImportedSceneObject(): Referenced light material in the scene hasn't been imported properly");
                }
                lightMaterial = std::make_shared<LightMaterial>(importedLightMaterialItr->second.name, importedLightMaterialItr->second.color, importedLightMaterialItr->second.intensity);
                instanceLightMaterials.add(object.light->lightMaterial, lightMaterial);
            } else 
            {
                lightMaterial = instanceLightMaterials.get(object.light->lightMaterial);
            }

            auto& cm = ComponentManager::getInstance();
            newSceneObject.second->assign(cm.create<ComponentPointLight>(std::make_shared<PointLight>(lightMaterial)));
        } else if (object.light->type == ImportedSceneLightType::MESH)
        {
            /* Import the light material as emissive material if needed */
            if (!instanceMaterials.isPresent(object.light->lightMaterial))
            {
                auto importedSceneLightMaterialItr = lights.find(object.light->lightMaterial);
                if (importedSceneLightMaterialItr == lights.end())
                {
                    throw std::runtime_error("addImportedSceneObject(): Referenced light material in the scene hasn't been imported properly");
                }

                const ImportedSceneLightMaterial& lm = importedSceneLightMaterialItr->second;

                /* Create a simple emissive lambert for the the mesh light material */
                auto mat = std::dynamic_pointer_cast<MaterialLambert>(m_vulkanWindow->getRenderer()->createMaterial(lm.name, MaterialType::MATERIAL_LAMBERT));
                mat->albedo() = glm::vec4(lm.color, 1);
                mat->emissive() = lm.intensity;
            }

            auto meshModel = m_vulkanWindow->getRenderer()->createVulkanMeshModel(sceneFolder + object.light->path);
            if (meshModel != nullptr) {
                if (object.light->submesh == -1 || object.light->submesh >= meshModel->getMeshes().size()) {
                    /* Imported mesh defines a path, but not a submesh. Create a new node and add all the meshes as children */
                    auto child = createEmptySceneObject("meshModel", Transform(), newSceneObject.first);
                    
                    addSceneObjectMeshes(child.first, sceneFolder + object.light->path, object.light->lightMaterial);
                } else{
                    /* Else, assign the corresponding mesh to the object */
                    auto mesh = meshModel->getMeshes()[object.light->submesh];

                    auto& cm = ComponentManager::getInstance();
                    newSceneObject.second->assign(cm.create<ComponentMesh>(mesh));
                    newSceneObject.second->assign(cm.create<ComponentMaterial>(instanceMaterials.get(object.light->lightMaterial)));
                }
            } else {
                utils::ConsoleWarning("addImportedSceneObject(): Failed to import: " + sceneFolder + object.mesh->path);
            }
        }
    }

    /* Add children */
    bool directionalLightDetected = false;
    for(auto itr : object.children)
    {
        directionalLightDetected = directionalLightDetected || addImportedSceneObject(itr, materials, lights, newSceneObject.first, sceneFolder);
    }

    return directionalLightDetected;
}

bool MainWindow::event(QEvent* event) 
{

    if (event->type() == QEvent::WindowActivate) 
    {
        m_vulkanWindow->windowActicated(true);      
    } 
    if (event->type() == QEvent::WindowDeactivate)
    {
        m_vulkanWindow->windowActicated(false);      
    }

    return QMainWindow::event(event);
}

void MainWindow::onImportModelSlot()
{   
    QString filename = QFileDialog::getOpenFileName(this,
        tr("Import model"), "./assets/models",
        tr("Model (*.obj *.fbx);;All Files (*)"));

    if (filename == "") return;

    bool ret = m_vulkanWindow->getRenderer()->createVulkanMeshModel(filename.toStdString()) != nullptr;
 
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
        auto tex = m_vulkanWindow->getRenderer()->createTexture(texture.toStdString(), VK_FORMAT_R8G8B8A8_SRGB);
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
        auto tex = m_vulkanWindow->getRenderer()->createTexture(texture.toStdString(), VK_FORMAT_R8G8B8A8_UNORM);
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

    auto tex = m_vulkanWindow->getRenderer()->createTextureHDR(filename.toStdString());
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

    auto envMap = m_vulkanWindow->getRenderer()->createEnvironmentMap(filename.toStdString());
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

    auto material = m_vulkanWindow->importMaterial(materialName, dirStd);

    if (material != nullptr) {
        if (m_selectedObjectWidgetMaterial != nullptr) m_selectedObjectWidgetMaterial->updateAvailableMaterials();
    }
}

void MainWindow::onImportMaterialZipStackSlot()
{
    QString filename = QFileDialog::getOpenFileName(this,
        tr("Import zip stack material"), "./assets/materials/",
        tr("Model (*.zip);;All Files (*)"));

    if (filename == "") return;

    std::string materialName = filename.split('/').back().toStdString();

    auto mat = m_vulkanWindow->importZipMaterial(materialName, filename.toStdString());
    if (mat != nullptr) {
        if (m_selectedObjectWidgetMaterial != nullptr) m_selectedObjectWidgetMaterial->updateAvailableMaterials();
    }
}

void MainWindow::onImportScene()
{    

    /* Select json file */
    QString sceneFile = QFileDialog::getOpenFileName(this,
        tr("Import scene"), "./assets/scenes/",
        tr("Model (*.json);;All Files (*)"));

    if (sceneFile == "") return;

    /* Parse json file */
    ImportedSceneCamera camera; 
    std::unordered_map<std::string, ImportedSceneMaterial> materials;
    std::unordered_map<std::string, ImportedSceneLightMaterial> lights;
    std::vector<ImportedSceneObject> sceneObjects;
    ImportedSceneEnvironment env;
    std::string sceneFolder;
    try {
        sceneFolder = importScene(sceneFile.toStdString(), camera, materials, lights, env, sceneObjects);
    } catch (std::runtime_error& e) {
        utils::ConsoleWarning("Unable to open scene file: " + std::string(e.what()));
        return;
    }

    m_vulkanWindow->getRenderer()->renderLoopActive(false);
    m_vulkanWindow->getRenderer()->waitIdle();

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

    /* Delete previous materials */
    AssetManager<std::string, Material>& instanceMaterials = AssetManager<std::string, Material>::getInstance();
    instanceMaterials.reset();
    /* Delete previous light materials */
    AssetManager<std::string, LightMaterial>& instanceLightMaterials = AssetManager<std::string, LightMaterial>::getInstance();
    instanceLightMaterials.reset();

    /* Remove old scene */
    m_sceneGraphWidget->blockSignals(true);
    while(m_sceneGraphWidget->topLevelItemCount() != 0)
    {
        removeObjectFromScene(m_sceneGraphWidget->topLevelItem(0));
    }
    m_selectedPreviousWidgetItem = nullptr;
    m_sceneGraphWidget->blockSignals(false);

    /* Create new scene */
    utils::ConsoleInfo("Importing models...");
    bool directionalLightDetected = false;
    for (auto& o : sceneObjects) 
    {
        directionalLightDetected = directionalLightDetected || addImportedSceneObject(o, materials, lights, nullptr, sceneFolder);
    }

    /* Fix directional light if none was detected in the scene */
    if (!directionalLightDetected)
    {
        auto directionalLightMaterial = std::make_shared<LightMaterial>("DirectionalLightMaterial", glm::vec3(1, 0.9, 0.8), 0.F);
        instanceLightMaterials.add(directionalLightMaterial->name, directionalLightMaterial);
        m_scene->getDirectionalLight()->lightMaterial = directionalLightMaterial;
    }

    /* Create and set the environment map */
    if (env.path != "") {
        auto envMap = m_vulkanWindow->getRenderer()->createEnvironmentMap(sceneFolder + env.path);
        if (envMap) {
            utils::ConsoleInfo("Environment map: " + sceneFolder + env.path + " set");

            AssetManager<std::string, MaterialSkybox>& materials = AssetManager<std::string, MaterialSkybox>::getInstance();
            auto material = materials.get("skybox");

            material->setMap(envMap);

            m_widgetEnvironment->updateMaps();
            m_widgetEnvironment->setEnvironmentType(EnvironmentType::HDRI);
        } 
    } else {
        m_widgetEnvironment->setEnvironmentType(EnvironmentType::SOLID_COLOR, true);
    }

    m_vulkanWindow->getRenderer()->renderLoopActive(true);
    utils::ConsoleInfo(sceneFile.toStdString() + " imported");
}

void MainWindow::onAddSceneObjectRootSlot()
{
    DialogAddSceneObject * dialog = new DialogAddSceneObject(nullptr, "Add an object to the scene", getImportedModels(), getCreatedMaterials());
    dialog->exec();
    std::string selectedModel = dialog->getSelectedModel();
    if (selectedModel == "") return;

    /* Create parent a oject for the mesh model */
    /* TODO set the name in some other way */
    auto parent = createEmptySceneObject("New object (" + std::to_string(m_nObjects++) + ")", dialog->getTransform(), nullptr);

    /* Add all the meshes of this mesh model as children of the parent item */
    addSceneObjectMeshes(parent.first, selectedModel, dialog->getSelectedMaterial());
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

    std::shared_ptr<SceneObject> parentObject = getSceneObject(selectedItem);

    /* Create parent a oject for the mesh model */
    /* TODO set the name in some other way */
    auto parent = createEmptySceneObject("New object (" + std::to_string(m_nObjects++) + ")", dialog->getTransform(), selectedItem);

    /* Add all the meshes of this mesh model as children of the parent item */
    addSceneObjectMeshes(parent.first, selectedModel, dialog->getSelectedMaterial());
}

void MainWindow::onRemoveSceneObjectSlot()
{
    /* Get the currently selected tree item */
    QTreeWidgetItem* selectedItem = m_sceneGraphWidget->currentItem();
    removeObjectFromScene(selectedItem);
}

void MainWindow::onAddMaterialSlot()
{
    DialogCreateMaterial * dialog = new DialogCreateMaterial(nullptr, "Create a material", getImportedModels());
    dialog->exec();

    if (dialog->m_selectedMaterialType == MaterialType::MATERIAL_NOT_SET) return;

    std::string materialName = dialog->m_selectedName.toStdString();
    if (materialName == "") {
        utils::ConsoleWarning("Material name can't be empty");
        return;
    }

    auto material = m_vulkanWindow->getRenderer()->createMaterial(materialName, dialog->m_selectedMaterialType);
    if (material == nullptr) {
        utils::ConsoleWarning("Failed to create material");
    } else {
        if (m_selectedObjectWidgetMaterial != nullptr) m_selectedObjectWidgetMaterial->updateAvailableMaterials();
    }
}

void MainWindow::onAddPointLightRootSlot()
{
    auto newObject = createEmptySceneObject("Point light", Transform(), nullptr);

    auto& lightMaterials = AssetManager<std::string, LightMaterial>::getInstance();
    if (!lightMaterials.isPresent("DefaultPointLightMaterial"))
    {
        lightMaterials.add("DefaultPointLightMaterial", std::make_shared<LightMaterial>("DefaultPointLightMaterial", glm::vec3(1, 1, 1), 1));
    }

    auto lightMat = lightMaterials.get("DefaultPointLightMaterial");

    auto& cm = ComponentManager::getInstance();
    newObject.second->assign(cm.create<ComponentPointLight>(std::make_shared<PointLight>(lightMat)));
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
    std::shared_ptr<SceneObject> selected = getSceneObject(selectedItem);

    /* Create new empty item under the selected item */
    auto newObject = createEmptySceneObject("Point light", Transform(), selectedItem);

    /* Add light component */
    auto& lightMaterials = AssetManager<std::string, LightMaterial>::getInstance();
    if (!lightMaterials.isPresent("DefaultPointLightMaterial"))
    {
        lightMaterials.add("DefaultPointLightMaterial", std::make_shared<LightMaterial>("DefaultPointLightMaterial", glm::vec3(1, 1, 1), 1));
    }

    auto lightMat = lightMaterials.get("DefaultPointLightMaterial");
    auto light = std::make_shared<PointLight>(lightMat);

    auto& cm = ComponentManager::getInstance();
    newObject.second->assign(cm.create<ComponentPointLight>(std::make_shared<PointLight>(lightMat)));
}

void MainWindow::onRenderSceneSlot()
{
    auto RTrenderer = m_vulkanWindow->getRenderer()->getRayTracingRenderer();
    
    if (!RTrenderer->isInitialized()) {
        int ret = QMessageBox::warning(this, tr("Error"),
            tr("GPU Ray tracing has not been initialized. Most likely, no ray tracing capable GPU has been found"),
            QMessageBox::Ok);

        return;
    }

    DialogSceneRender* dialog = new DialogSceneRender(nullptr, RTrenderer);
    dialog->exec();

    std::string filename = dialog->getRenderOutputFileName();
    if (filename == "") return;

    RTrenderer->setSamples(dialog->getSamples());
    RTrenderer->setMaxDepth(dialog->getDepth());
    RTrenderer->setRenderOutputFileName(dialog->getRenderOutputFileName());
    RTrenderer->setRenderOutputFileType(dialog->getRenderOutputFileType());
    RTrenderer->setRenderResolution(dialog->getResolutionWidth(), dialog->getResolutionHeight());
    delete dialog;

    auto ret = m_vulkanWindow->getRenderer()->renderRT();

    if (ret) {
        std::string fileExtension = (RTrenderer->getRenderOutputFileType() == OutputFileType::PNG) ? "png" : "hdr";
        std::string filename = RTrenderer->getRenderOutputFileName() + "." + fileExtension;

        utils::ConsoleInfo("Scene rendered: " + filename + " in: " + std::to_string(ret) + "ms");
    }
}

void MainWindow::onExportSceneSlot()
{
    DialogSceneExport* dialog = new DialogSceneExport(nullptr);
    dialog->exec();

    std::string folderName = dialog->getDestinationFolderName();
    if (folderName == "") return;

    ExportRenderParams params;
    params.name = dialog->getDestinationFolderName();
    params.fileTypes.push_back(dialog->getFileType());
    params.width = dialog->getResolutionWidth();
    params.height = dialog->getResolutionHeight();
    params.samples = dialog->getSamples();
    params.depth = dialog->getDepth();
    params.rdepth = dialog->getRDepth();
    params.hideBackground = dialog->getBackground();
    params.backgroundColor = dialog->getBackgroundColor();
    params.tessellation = dialog->getTessellation();
    params.parallax = dialog->getParallax();
    params.focalLength = dialog->getFocalLength();
    params.apertureSides = dialog->getApertureSides();
    params.aperture = dialog->getAperture();

    m_scene->exportScene(params);

    utils::ConsoleInfo("Scene exported: " + folderName);

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
    std::shared_ptr<SceneObject> object = getSceneObject(selectedItem);

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

void MainWindow::onStartUpInitialization()
{
    AssetManager<std::string, MeshModel>& instanceModels = AssetManager<std::string, MeshModel>::getInstance();
    AssetManager<std::string, Material>& instanceMaterials = AssetManager<std::string, Material>::getInstance();
    AssetManager<std::string, LightMaterial>& instanceLightMaterials = AssetManager<std::string, LightMaterial>::getInstance();

    try {
        auto mat = instanceMaterials.get("defaultMaterial");
        auto sphere = instanceModels.get("assets/models/uvsphere.obj");
        auto plane = instanceModels.get("assets/models/plane.obj");
        auto lightMaterial = instanceLightMaterials.get("DefaultPointLightMaterial");

        auto& cm = ComponentManager::getInstance();
        
        auto o1 = createEmptySceneObject("sphere", Transform(), nullptr);
        o1.second->assign(cm.create<ComponentMesh>(sphere->getMeshes()[0]));
        o1.second->assign(cm.create<ComponentMaterial>(mat));
        
        auto o2 = createEmptySceneObject("plane", Transform({0, -1, 0},{3, 3, 3}), nullptr);
        o2.second->assign(cm.create<ComponentMesh>(plane->getMeshes()[0]));
        o2.second->assign(cm.create<ComponentMaterial>(mat));
    } catch (std::exception& e) {
        utils::ConsoleCritical("Failed to setup initialization scene: " + std::string(e.what()));
    }
}
