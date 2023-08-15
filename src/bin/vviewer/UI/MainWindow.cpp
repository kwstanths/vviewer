#include "MainWindow.hpp"

#include <cstddef>
#include <memory>
#include <qaction.h>
#include <qdebug.h>
#include <qtreewidget.h>
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

#include "UI/widgets/WidgetRightPanel.hpp"
#include "UI/widgets/WidgetSceneGraph.hpp"
#include "core/SceneObject.hpp"
#include "dialogs/DialogAddSceneObject.hpp"
#include "dialogs/DialogCreateMaterial.hpp"
#include "dialogs/DialogSceneExport.hpp"
#include "dialogs/DialogSceneRender.hpp"
#include "dialogs/DialogWaiting.hpp"
#include "UIUtils.hpp"

#include "core/AssetManager.hpp"
#include "core/Import.hpp"
#include "core/Lights.hpp"
#include "core/Materials.hpp"
#include "math/Transform.hpp"
#include "utils/ECS.hpp"
#include "utils/Tasks.hpp"
#include "vulkan/VulkanMaterials.hpp"
#include "vulkan/renderers/VulkanRendererRayTracing.hpp"

MainWindow::MainWindow(QWidget *parent) :QMainWindow(parent) {
    
    QWidget * widgetVulkan = initVulkanWindowWidget();

    QHBoxLayout * layout_main = new QHBoxLayout();
    layout_main->addWidget(initLeftPanel());
    layout_main->addWidget(widgetVulkan);
    layout_main->addWidget(initRightPanel());

    QWidget * widget_main = new QWidget();
    widget_main->setLayout(layout_main);

    m_scene = m_vulkanWindow->engine()->scene();

    createMenu();

    setCentralWidget(widget_main);
    resize(1600, 1100);
}

MainWindow::~MainWindow() {

}

QWidget * MainWindow::initLeftPanel()
{

    m_sceneGraphWidget = new WidgetSceneGraph(this);
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

QWidget * MainWindow::initRightPanel()
{
    m_widgetRightPanel = new WidgetRightPanel(this, m_vulkanWindow->engine());
    connect(m_widgetRightPanel, &WidgetRightPanel::selectedSceneObjectNameChanged, this, &MainWindow::onSelectedSceneObjectNameChangedSlot);
    connect(m_vulkanWindow, &VulkanWindow::selectedObjectPositionChanged, m_widgetRightPanel, &WidgetRightPanel::onTransformChanged);

    m_widgetRightPanel->setFixedWidth(380);
    return m_widgetRightPanel;
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

    QAction * actionCreateMaterial = new QAction(tr("&Create a material"), this);
    actionCreateMaterial->setStatusTip(tr("Create a material"));
    connect(actionCreateMaterial, &QAction::triggered, this, &MainWindow::onAddMaterialSlot);

    QAction* actionRender = new QAction(tr("&Render scene"), this);
    actionRender->setStatusTip(tr("Render scene"));
    connect(actionRender, &QAction::triggered, this, &MainWindow::onRenderSceneSlot);

    QAction* actionExport = new QAction(tr("&Export scene"), this);
    actionExport->setStatusTip(tr("Export scene"));
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
    QMenu * m_menuAdd = menuBar()->addMenu(tr("&Create"));
    m_menuAdd->addAction(actionCreateMaterial);
    QMenu* m_menuRender = menuBar()->addMenu(tr("&Render"));
    m_menuRender->addAction(actionRender);
    QMenu* m_menuExport = menuBar()->addMenu(tr("&Export"));
    m_menuExport->addAction(actionExport);
}

std::pair<QTreeWidgetItem*, std::shared_ptr<SceneObject>> MainWindow::createEmptySceneObject(std::string name, const Transform& transform, QTreeWidgetItem* parent)
{
    /* If parent is null, add at root */
    if (parent == nullptr)
    {
        std::shared_ptr<SceneObject> newObject = m_scene->addSceneObject(name, transform);
        QTreeWidgetItem* widgetItem = m_sceneGraphWidget->createTreeWidgetItem(newObject);

        m_sceneGraphWidget->addTopLevelItem(widgetItem); 

        return {widgetItem, newObject};
    } else {
        std::shared_ptr<SceneObject> parentObject = WidgetSceneGraph::getSceneObject(parent);

        std::shared_ptr<SceneObject> newObject = m_scene->addSceneObject(name, parentObject, transform);
        QTreeWidgetItem* widgetItem = m_sceneGraphWidget->createTreeWidgetItem(newObject);

        parent->addChild(widgetItem);

        return {widgetItem, newObject};
    }
}

void MainWindow::selectObject(QTreeWidgetItem* selectedItem)
{
    QTreeWidgetItem * previouslySelected = m_sceneGraphWidget->getPreviouslySelectedItem();
    /* If it's the previously selected item, return */
    if (selectedItem == previouslySelected) return;

    m_sceneGraphWidget->blockSignals(true);

    /* Get selected object */
    std::shared_ptr<SceneObject> sceneObject = WidgetSceneGraph::getSceneObject(selectedItem);
    /* Remove UI selection if needed */
    if (!selectedItem->isSelected()) {
        selectedItem->setSelected(true);
        selectedItem->setExpanded(true);
    }

    /* Remove selection from previously selected item */
    if (previouslySelected != nullptr) {
        previouslySelected->setSelected(false);
        std::shared_ptr<SceneObject> previousSelected = WidgetSceneGraph::getSceneObject(previouslySelected);
        previousSelected->m_isSelected = false;
    }

    /* Set selection to new item */
    m_sceneGraphWidget->setPreviouslySelectedItem(selectedItem);
    sceneObject->m_isSelected = true;
    m_vulkanWindow->engine()->renderer()->setSelectedObject(sceneObject);

    m_widgetRightPanel->setSelectedObject(sceneObject);

    m_sceneGraphWidget->blockSignals(false);
}

void MainWindow::removeObjectFromScene(QTreeWidgetItem* treeItem)
{
    /* Get corresponding scene object */
    std::shared_ptr<SceneObject> selectedObject = WidgetSceneGraph::getSceneObject(treeItem);
    
    /* Remove scene object from scene */
    m_scene->removeSceneObject(selectedObject);

    /* Set selected to null */
    m_vulkanWindow->engine()->renderer()->setSelectedObject(nullptr);

    /* Remove from scene graph UI */
    m_sceneGraphWidget->removeItem(treeItem);
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
    
    std::shared_ptr<SceneObject> parentObject = WidgetSceneGraph::getSceneObject(parentItem);

    /* For all meshes inside the mesh model, add them in the scene and the UI */
    for (auto& m : modelMeshes) {
        auto child = createEmptySceneObject(m->m_name, Transform(), parentItem);

        child.second->add<ComponentMesh>().mesh = m;
        child.second->add<ComponentMaterial>().material = instanceMaterials.get(material);
    }
}

void MainWindow::addImportedSceneObject(const ImportedSceneObject& object, 
            const std::unordered_map<std::string, ImportedSceneMaterial>& materials,
            const std::unordered_map<std::string, ImportedSceneLightMaterial>& lights,
            QTreeWidgetItem * parentItem, std::string sceneFolder)
{
    AssetManager<std::string, Material>& instanceMaterials = AssetManager<std::string, Material>::getInstance();
    AssetManager<std::string, LightMaterial>& instanceLightMaterials = AssetManager<std::string, LightMaterial>::getInstance();

    /* Create new scene object */
    auto newSceneObject = createEmptySceneObject(object.name, object.transform, parentItem);

    if (object.mesh.has_value() && object.light.has_value() && object.light->type == ImportedSceneLightType::MESH)
    {
        throw std::runtime_error("addImportedSceneObject(): Imported scene node has both a mesh component and a light mesh component");
    }

    VulkanEngine * engine = m_vulkanWindow->engine();

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
                auto mat = std::dynamic_pointer_cast<MaterialLambert>(engine->materials().createMaterial(m.name, MaterialType::MATERIAL_LAMBERT));

                mat->albedo() = glm::vec4(m.albedo, 1);
                if (m.albedoTexture != "") {
                    auto tex = engine->textures().createTexture(sceneFolder + m.albedoTexture, VK_FORMAT_R8G8B8A8_SRGB);
                    mat->setAlbedoTexture(tex);
                }
                if (m.normalTexture != "") {
                    auto tex = engine->textures().createTexture(sceneFolder + m.normalTexture, VK_FORMAT_R8G8B8A8_UNORM);
                    mat->setNormalTexture(tex);
                }

                mat->uTiling() = m.scale.x;
                mat->vTiling() = m.scale.y;
            } 
            else if (m.type == ImportedSceneMaterialType::DISNEY)
            {
                auto mat = std::dynamic_pointer_cast<MaterialPBRStandard>(engine->materials().createMaterial(m.name, MaterialType::MATERIAL_PBR_STANDARD));

                mat->albedo() = glm::vec4(m.albedo, 1);
                if (m.albedoTexture != "") {
                    auto tex = engine->textures().createTexture(sceneFolder + m.albedoTexture, VK_FORMAT_R8G8B8A8_SRGB);
                    if (tex != nullptr) mat->setAlbedoTexture(tex);
                }
                mat->roughness() = m.roughness;
                if (m.roughnessTexture != "") {
                    auto tex = engine->textures().createTexture(sceneFolder + m.roughnessTexture, VK_FORMAT_R8G8B8A8_UNORM);
                    if (tex != nullptr) mat->setRoughnessTexture(tex);
                }
                mat->metallic() = m.metallic;
                if (m.metallicTexture != "") {
                    auto tex = engine->textures().createTexture(sceneFolder + m.metallicTexture, VK_FORMAT_R8G8B8A8_UNORM);
                    if (tex != nullptr) mat->setMetallicTexture(tex);
                }
                if (m.normalTexture != "") {
                    auto tex = engine->textures().createTexture(sceneFolder + m.normalTexture, VK_FORMAT_R8G8B8A8_UNORM);
                    if (tex != nullptr) mat->setNormalTexture(tex);
                }
                
                mat->uTiling() = m.scale.x;
                mat->vTiling() = m.scale.y;
            }
        }

        auto meshModel = static_cast<VulkanRenderer *>(engine->renderer())->createVulkanMeshModel(sceneFolder + object.mesh->path);
        if (meshModel != nullptr) {
            if (object.mesh->submesh == -1 || object.mesh->submesh >= meshModel->getMeshes().size()) {
                /* Imported mesh defines a path, but not a submesh. Create a new node and add all the meshes as children */
                auto child = createEmptySceneObject("meshModel", Transform(), newSceneObject.first);
                
                addSceneObjectMeshes(child.first, sceneFolder + object.mesh->path, object.mesh->material);
            } else{
                /* Else, assign the corresponding mesh to the object */
                auto mesh = meshModel->getMeshes()[object.mesh->submesh];
                newSceneObject.second->add<ComponentMesh>().mesh = mesh;

                auto mat = instanceMaterials.get(object.mesh->material);
                newSceneObject.second->add<ComponentMaterial>().material = mat;
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

            newSceneObject.second->add<ComponentLight>().light = std::make_shared<Light>(object.name,  LightType::POINT_LIGHT, lightMaterial);
        } 
        else if (object.light->type == ImportedSceneLightType::DISTANT)
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

            newSceneObject.second->add<ComponentLight>().light = std::make_shared<Light>(object.name,  LightType::DIRECTIONAL_LIGHT, lightMaterial);
        }
        else if (object.light->type == ImportedSceneLightType::MESH)
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
                auto mat = std::dynamic_pointer_cast<MaterialLambert>(engine->materials().createMaterial(lm.name, MaterialType::MATERIAL_LAMBERT));
                mat->albedo() = glm::vec4(lm.color, 1);
                mat->emissive() = lm.intensity;
            }

            auto meshModel = static_cast<VulkanRenderer *>(engine->renderer())->createVulkanMeshModel(sceneFolder + object.light->path);
            if (meshModel != nullptr) {
                if (object.light->submesh == -1 || object.light->submesh >= meshModel->getMeshes().size()) {
                    /* Imported mesh defines a path, but not a submesh. Create a new node and add all the meshes as children */
                    auto child = createEmptySceneObject("meshModel", Transform(), newSceneObject.first);
                    
                    addSceneObjectMeshes(child.first, sceneFolder + object.light->path, object.light->lightMaterial);
                } else{
                    /* Else, assign the corresponding mesh to the object */
                    auto mesh = meshModel->getMeshes()[object.light->submesh];

                    newSceneObject.second->add<ComponentMesh>().mesh = mesh;
                    newSceneObject.second->add<ComponentMaterial>().material = instanceMaterials.get(object.light->lightMaterial);
                }
            } else {
                utils::ConsoleWarning("addImportedSceneObject(): Failed to import: " + sceneFolder + object.mesh->path);
            }
        }
    }

    /* Add children */
    for(auto itr : object.children)
    {
        addImportedSceneObject(itr, materials, lights, newSceneObject.first, sceneFolder);
    }

    return;
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

    struct ImportFunct {
        ImportFunct(VulkanRenderer * r, std::string f) : renderer(r), filename(f) {} ;
        VulkanRenderer * renderer;
        std::string filename;
        bool operator () (float&) {
            bool ret = renderer->createVulkanMeshModel(filename) != nullptr;
            if (ret) {
                utils::ConsoleInfo("Model imported");
            }
            return ret;
        }
    };
    Task task;
    task.f = ImportFunct(static_cast<VulkanRenderer *>(m_vulkanWindow->engine()->renderer()), filename.toStdString());

    DialogWaiting * waiting = new DialogWaiting(nullptr, "Importing...", &task);
    waiting->exec();

    delete waiting;
}

void MainWindow::onImportTextureColorSlot()
{
    QStringList filenames = QFileDialog::getOpenFileNames(this,
        tr("Import textures"), "./assets",
        tr("Textures (*.tga, *.png);;All Files (*)"));

    if (filenames.length() == 0) return;

    struct ImportFunct {
        ImportFunct(VulkanTextures& t, QStringList& fs) : textures(t), filenames(fs) {} ;
        VulkanTextures& textures;
        QStringList filenames;
    
        bool operator () (float& progress) {
            bool success = true;
            for (uint32_t t = 0; t < filenames.length(); t++)
            {
                const auto& texture = filenames[t];
                auto tex = textures.createTexture(texture.toStdString(), VK_FORMAT_R8G8B8A8_SRGB);
                if (tex) {
                    utils::ConsoleInfo("Texture: " + texture.toStdString() + " imported");
                } else {
                    success = false;
                }
                progress = static_cast<float>(t) / static_cast<float>(filenames.length());
            }
            return success;
        }
    };
    Task task;
    task.f = ImportFunct(m_vulkanWindow->engine()->textures(), filenames);

    DialogWaiting * waiting = new DialogWaiting(nullptr, "Importing...", &task);
    waiting->exec();

    delete waiting;
}

void MainWindow::onImportTextureOtherSlot()
{
    QStringList filenames = QFileDialog::getOpenFileNames(this,
        tr("Import textures"), "./assets",
        tr("Textures (*.tga, *.png);;All Files (*)"));

    if (filenames.length() == 0) return;

    struct ImportFunct {
        ImportFunct(VulkanTextures& t, QStringList& fs) : textures(t), filenames(fs) {} ;
        VulkanTextures& textures;
        QStringList filenames;
    
        bool operator () (float& progress) {
            bool success = true;
            for (uint32_t t = 0; t < filenames.length(); t++)
            {
                const auto& texture = filenames[t];
                auto tex = textures.createTexture(texture.toStdString(), VK_FORMAT_R8G8B8A8_UNORM);
                if (tex) {
                    utils::ConsoleInfo("Texture: " + texture.toStdString() + " imported");
                } else {
                    success = false;
                }
                progress = static_cast<float>(t) / static_cast<float>(filenames.length());
            }
            return success;
        }
    };
    Task task;
    task.f = ImportFunct(m_vulkanWindow->engine()->textures(), filenames);

    DialogWaiting * waiting = new DialogWaiting(nullptr, "Importing...", &task);
    waiting->exec();

    delete waiting;
}

void MainWindow::onImportTextureHDRSlot()
{
    QString filename = QFileDialog::getOpenFileName(this,
        tr("Import HDR texture"), "./assets/HDR/",
        tr("Model (*.hdr);;All Files (*)"));

    if (filename == "") return;

    struct ImportFunct {
        ImportFunct(VulkanTextures& t, std::string f) : textures(t), filename(f) {} ;
        VulkanTextures& textures;
        std::string filename;
        bool operator () (float&) {
            auto tex = textures.createTextureHDR(filename);
            if (tex) {
                utils::ConsoleInfo("Texture: " + filename + " imported");
                return true;
            }
            return false;
        }
    };
    Task task;
    task.f = ImportFunct(m_vulkanWindow->engine()->textures(), filename.toStdString());

    DialogWaiting * waiting = new DialogWaiting(nullptr, "Importing...", &task);
    waiting->exec();

    delete waiting;
}

void MainWindow::onImportEnvironmentMap()
{
    QString filename = QFileDialog::getOpenFileName(this,
        tr("Import HDR texture"), "./assets/HDR/",
        tr("Model (*.hdr);;All Files (*)"));

    if (filename == "") return;

    struct ImportFunct {
        ImportFunct(VulkanRenderer * r, std::string f) : renderer(r), filename(f) {} ;
        VulkanRenderer * renderer;
        std::string filename;
        bool operator () (float&) {
            auto envMap = renderer->createEnvironmentMap(filename);
            if (envMap) {
                utils::ConsoleInfo("Environment map: " + filename + " imported");
                return true;
            }
            return false;
        }
    };
    Task task;
    task.f = ImportFunct(static_cast<VulkanRenderer *>(m_vulkanWindow->engine()->renderer()), filename.toStdString());

    DialogWaiting * waiting = new DialogWaiting(nullptr, "Importing...", &task);
    waiting->exec();

    m_widgetRightPanel->getEnvironmentWidget()->updateMaps();

    delete waiting;
}

void MainWindow::onImportMaterial()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Choose directory", "./assets/materials/", QFileDialog::ShowDirsOnly);
    if (dir == "") return;

    std::string materialName = dir.split('/').back().toStdString();
    std::string dirStd = dir.toStdString();

    struct ImportFunct {
        ImportFunct(VulkanWindow * vkw, std::string d, std::string n) : vkwindow(vkw), dir(d), materialName(n) {} ;
        VulkanWindow * vkwindow;
        std::string dir;
        std::string materialName;
        bool operator () (float&) {
            auto material = vkwindow->importMaterial(materialName, dir);
            if (material) {
                utils::ConsoleInfo("Material: " + materialName + " has been created");
                return true;
            } else {
                return false;
            }
        }
    };
    Task task;
    task.f = ImportFunct(m_vulkanWindow, dirStd, materialName);

    DialogWaiting * waiting = new DialogWaiting(nullptr, "Importing...", &task);
    waiting->exec();

    if (task.success) {
        m_widgetRightPanel->updateAvailableMaterials();
    }

    delete waiting;
}

void MainWindow::onImportMaterialZipStackSlot()
{
    QString filename = QFileDialog::getOpenFileName(this,
        tr("Import zip stack material"), "./assets/materials/",
        tr("Model (*.zip);;All Files (*)"));

    if (filename == "") return;

    std::string materialName = filename.split('/').back().toStdString();

    struct ImportFunct {
        ImportFunct(VulkanWindow * vkw, std::string f, std::string n) : vkwindow(vkw), filename(f), materialName(n) {} ;
        VulkanWindow * vkwindow;
        std::string filename;
        std::string materialName;
        bool operator () (float&) {
            auto material = vkwindow->importZipMaterial(materialName, filename);
            if (material) {
                utils::ConsoleInfo("Material: " + materialName + " has been created");
                return true;
            } else {
                return false;
            }
        }
    };
    Task task;
    task.f = ImportFunct(m_vulkanWindow, filename.toStdString(), materialName);

    DialogWaiting * waiting = new DialogWaiting(nullptr, "Importing...", &task);
    waiting->exec();

    if (task.success) {
        m_widgetRightPanel->updateAvailableMaterials();
    }

    delete waiting;
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

    m_vulkanWindow->engine()->stop();
    m_vulkanWindow->engine()->waitIdle();

    /* Set camera */
    {
        auto newCamera = std::make_shared<PerspectiveCamera>();
        Transform& cameraTransform = newCamera->getTransform();
        cameraTransform.setPosition(camera.position);
        cameraTransform.setRotation(glm::normalize(camera.target - camera.position), camera.up);

        newCamera->setFoV(camera.fov);
        newCamera->setWindowSize(m_vulkanWindow->size().width(), m_vulkanWindow->size().height());

        m_scene->setCamera(newCamera);
        m_widgetRightPanel->getEnvironmentWidget()->setCamera(newCamera);
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
    m_sceneGraphWidget->setPreviouslySelectedItem(nullptr);
    m_sceneGraphWidget->blockSignals(false);

    /* Create new scene */
    utils::ConsoleInfo("Importing models...");
    for (auto& o : sceneObjects) 
    {
        addImportedSceneObject(o, materials, lights, nullptr, sceneFolder);
    }

    VulkanRenderer * renderer = static_cast<VulkanRenderer *>(m_vulkanWindow->engine()->renderer());    

    /* Create and set the environment map */
    if (env.path != "") {
        auto envMap = renderer->createEnvironmentMap(sceneFolder + env.path);
        if (envMap) {
            utils::ConsoleInfo("Environment map: " + sceneFolder + env.path + " set");

            AssetManager<std::string, MaterialSkybox>& materials = AssetManager<std::string, MaterialSkybox>::getInstance();
            auto material = materials.get("skybox");

            material->setMap(envMap);

            m_widgetRightPanel->getEnvironmentWidget()->updateMaps();
            m_widgetRightPanel->getEnvironmentWidget()->setEnvironmentType(EnvironmentType::HDRI);
        } 
    } else {
        m_widgetRightPanel->getEnvironmentWidget()->setEnvironmentType(EnvironmentType::SOLID_COLOR, true);
    }

    m_vulkanWindow->engine()->start();
    utils::ConsoleInfo(sceneFile.toStdString() + " imported");
}

void MainWindow::onAddEmptyObjectSlot()
{
    /* Get currently selected tree item */
    QTreeWidgetItem* selectedItem = m_sceneGraphWidget->currentItem();
    
    m_vulkanWindow->engine()->stop();
    m_vulkanWindow->engine()->waitIdle();

    /* Create parent a oject for the mesh model */
    /* TODO set the name in some other way */
    auto parent = createEmptySceneObject("New object (" + std::to_string(m_nObjects++) + ")", Transform(), selectedItem);

    m_vulkanWindow->engine()->start();
}

void MainWindow::onAddSceneObjectSlot()
{
    /* Get currently selected tree item */
    QTreeWidgetItem* selectedItem = m_sceneGraphWidget->currentItem();

    DialogAddSceneObject* dialog = new DialogAddSceneObject(nullptr, "Add an object to the scene", getImportedModels(), getCreatedMaterials());
    dialog->exec();
    std::string selectedModel = dialog->getSelectedModel();
    if (selectedModel == "") return;

    m_vulkanWindow->engine()->stop();
    m_vulkanWindow->engine()->waitIdle();

    /* Create parent a oject for the mesh model */
    /* TODO set the name in some other way */
    auto parent = createEmptySceneObject("New object (" + std::to_string(m_nObjects++) + ")", dialog->getTransform(), selectedItem);

    /* Add all the meshes of this mesh model as children of the parent item */
    addSceneObjectMeshes(parent.first, selectedModel, dialog->getSelectedMaterial());

    m_vulkanWindow->engine()->start();
}

void MainWindow::onRemoveSceneObjectSlot()
{
    /* Get the currently selected tree item */
    QTreeWidgetItem* selectedItem = m_sceneGraphWidget->currentItem();

    if (selectedItem == nullptr) return;

    m_vulkanWindow->engine()->stop();
    m_vulkanWindow->engine()->waitIdle();

    removeObjectFromScene(selectedItem);

    m_vulkanWindow->engine()->start();
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

    auto material = m_vulkanWindow->engine()->materials().createMaterial(materialName, dialog->m_selectedMaterialType);
    if (material == nullptr) {
        utils::ConsoleWarning("Failed to create material");
    } else {
        m_widgetRightPanel->updateAvailableMaterials();
    }
}

void MainWindow::onAddPointLightSlot()
{
    /* Get currently selected tree item */
    QTreeWidgetItem* selectedItem = m_sceneGraphWidget->currentItem();

    m_vulkanWindow->engine()->stop();
    m_vulkanWindow->engine()->waitIdle();

    /* Create new empty item under the selected item */
    auto newObject = createEmptySceneObject("Point light", Transform(), selectedItem);

    /* Add light component */
    auto& lightMaterials = AssetManager<std::string, LightMaterial>::getInstance();
    if (!lightMaterials.isPresent("DefaultPointLightMaterial"))
    {
        lightMaterials.add("DefaultPointLightMaterial", std::make_shared<LightMaterial>("DefaultPointLightMaterial", glm::vec3(1, 1, 1), 1));
    }
    auto lightMat = lightMaterials.get("DefaultPointLightMaterial");

    newObject.second->add<ComponentLight>().light = std::make_shared<Light>(newObject.second->m_name, LightType::POINT_LIGHT, lightMat);

    m_vulkanWindow->engine()->start();
}

void MainWindow::onRenderSceneSlot()
{
    VulkanRenderer * renderer = static_cast<VulkanRenderer *>(m_vulkanWindow->engine()->renderer());
    auto& RTrenderer = renderer->getRayTracingRenderer();
    
    if (!RTrenderer.isInitialized()) {
        int ret = QMessageBox::warning(this, tr("Error"),
            tr("GPU Ray tracing has not been initialized. Most likely, no ray tracing capable GPU has been found"),
            QMessageBox::Ok);

        return;
    }

    m_vulkanWindow->engine()->stop();

    DialogSceneRender* dialog = new DialogSceneRender(nullptr, RTrenderer);
    dialog->exec();

    std::string filename = dialog->getRenderOutputFileName();
    if (filename == "") {
        m_vulkanWindow->engine()->start();
        return;
    }

    RTrenderer.setSamples(dialog->getSamples());
    RTrenderer.setMaxDepth(dialog->getDepth());
    RTrenderer.setRenderOutputFileName(dialog->getRenderOutputFileName());
    RTrenderer.setRenderOutputFileType(dialog->getRenderOutputFileType());
    RTrenderer.setRenderResolution(dialog->getResolutionWidth(), dialog->getResolutionHeight());
    delete dialog;

    struct RTRenderTask : public Task {
        VulkanRenderer * renderer;
        RTRenderTask(VulkanRenderer * r) : renderer(r) { 
            f = Funct(renderer);
        };

        struct Funct {
            Funct(VulkanRenderer * r) : renderer(r) {} ;
            VulkanRenderer * renderer;
            bool operator () (float&) {
                renderer->renderRT();
                return true;
            }
        };

        float getProgress() const override { return renderer->getRayTracingRenderer().getRenderProgress(); }
    };
    auto task = RTRenderTask(renderer);

    /* Wait for the render loop to idle */
    m_vulkanWindow->engine()->waitIdle();

    /* Spawn rendering RTRenderTask */
    DialogWaiting * waiting = new DialogWaiting(nullptr, "Rendering...", &task);
    waiting->exec();

    m_vulkanWindow->engine()->start();

    delete waiting;
    
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

void MainWindow::onDuplicateSceneObjectSlot()
{
    /* Get currently selected tree item */
    QTreeWidgetItem* selectedItem = m_sceneGraphWidget->currentItem();

    std::function<void(QTreeWidgetItem*, QTreeWidgetItem*)> duplicate;
    duplicate = [&](QTreeWidgetItem* duplicatedItem, QTreeWidgetItem* parent) { 
        std::shared_ptr<SceneObject> so = m_sceneGraphWidget->getSceneObject(duplicatedItem);
        auto newObject = createEmptySceneObject(so->m_name + " (D)", so->m_localTransform, parent);

        if (so->has<ComponentLight>())
        {
            auto l = so->get<ComponentLight>().light;
            newObject.second->add<ComponentLight>().light = std::make_shared<Light>(newObject.second->m_name, l->type, l->lightMaterial); 
        }
        if (so->has<ComponentMesh>())
        {
            newObject.second->add<ComponentMesh>().mesh = so->get<ComponentMesh>().mesh;
        }
        if (so->has<ComponentMaterial>())
        {
            newObject.second->add<ComponentMaterial>().material = so->get<ComponentMaterial>().material;
        }

        for(int c = 0; c < duplicatedItem->childCount(); c++)
        {
            duplicate(duplicatedItem->child(c), newObject.first);
        }
    };

    m_vulkanWindow->engine()->stop();
    m_vulkanWindow->engine()->waitIdle();

    duplicate(selectedItem, selectedItem->parent());

    m_vulkanWindow->engine()->start();
}

void MainWindow::onSelectedSceneObjectChangedSlotUI()
{
    /* Get currently selected object, and the corresponding SceneObject */
    QTreeWidgetItem * selectedItem = m_sceneGraphWidget->currentItem();
    /* When all items have been removed, this function is called with a null object selected */

    if (selectedItem == nullptr) {
        return;
    }

    selectObject(selectedItem);
}

void MainWindow::onSelectedSceneObjectChangedSlot3DScene(std::shared_ptr<SceneObject> object)
{
    QTreeWidgetItem* treeItem = m_sceneGraphWidget->getTreeWidgetItem(object);
    if (treeItem == nullptr) return;

    selectObject(treeItem);
}

void MainWindow::onSelectedSceneObjectNameChangedSlot(QString newName)
{
    QTreeWidgetItem* selectedItem = m_sceneGraphWidget->currentItem();
    std::shared_ptr<SceneObject> object = WidgetSceneGraph::getSceneObject(selectedItem);

    object->m_name = newName.toStdString();
    selectedItem->setText(0, newName);
}

void MainWindow::onContextMenuSceneGraph(const QPoint& pos)
{
    QMenu contextMenu(tr("Context menu"), this);

    QAction action1("Add empty", this);
    connect(&action1, SIGNAL(triggered()), this, SLOT(onAddEmptyObjectSlot()));
    contextMenu.addAction(&action1);
    QAction action2("Add scene object", this);
    connect(&action2, SIGNAL(triggered()), this, SLOT(onAddSceneObjectSlot()));
    contextMenu.addAction(&action2);
    QAction action3("Remove scene object", this);
    connect(&action3, SIGNAL(triggered()), this, SLOT(onRemoveSceneObjectSlot()));
    contextMenu.addAction(&action3);
    QAction action4("Add point light", this);
    connect(&action4, SIGNAL(triggered()), this, SLOT(onAddPointLightSlot()));
    contextMenu.addAction(&action4);
    
    /* Check if it's a valid index */
    QModelIndex index = m_sceneGraphWidget->indexAt(pos);
    QAction * action5 = nullptr;
    if(index.isValid())
    {
        action5 = new QAction("Duplicate", this);
        connect(action5, SIGNAL(triggered()), this, SLOT(onDuplicateSceneObjectSlot()));
        contextMenu.addAction(action5);
    }

    contextMenu.exec(m_sceneGraphWidget->mapToGlobal(pos));
    if (action5 != nullptr) delete action5;
}

void MainWindow::onStartUpInitialization()
{
    AssetManager<std::string, MeshModel>& instanceModels = AssetManager<std::string, MeshModel>::getInstance();
    AssetManager<std::string, Material>& instanceMaterials = AssetManager<std::string, Material>::getInstance();
    AssetManager<std::string, LightMaterial>& instanceLightMaterials = AssetManager<std::string, LightMaterial>::getInstance();

    /* Create Ball an plane scene */
    try {
        auto matDef = instanceMaterials.get("defaultMaterial");
        auto sphere = instanceModels.get("assets/models/uvsphere.obj");
        auto plane = instanceModels.get("assets/models/plane.obj");

        std::static_pointer_cast<VulkanMaterialPBRStandard>(matDef)->metallic() = 0.5;
        std::static_pointer_cast<VulkanMaterialPBRStandard>(matDef)->roughness() = 0.5;
    
        auto o1 = createEmptySceneObject("sphere", Transform({0, 0, 0}, {1, 1, 1}), nullptr);
        o1.second->add<ComponentMesh>().mesh = sphere->getMeshes()[0];
        o1.second->add<ComponentMaterial>().material = matDef;

        auto o2 = createEmptySceneObject("plane", Transform({0, -1, 0}, {3, 3, 3}), nullptr);
        o2.second->add<ComponentMesh>().mesh = plane->getMeshes()[0];
        o2.second->add<ComponentMaterial>().material = matDef;

    } catch (std::exception& e) {
        utils::ConsoleCritical("Failed to setup initialization scene: " + std::string(e.what()));
    }
    
}
