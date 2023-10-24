#include "MainWindow.hpp"

#include <cstddef>
#include <memory>
#include <qaction.h>
#include <qdebug.h>
#include <qtreewidget.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qmenubar.h>
#include <qfiledialog.h>
#include <qvariant.h>
#include <qmessagebox.h>

#include <glm/glm.hpp>

#include <string>
#include <debug_tools/Console.hpp>

#include "UIUtils.hpp"
#include "VulkanViewportWindow.hpp"
#include "UI/widgets/WidgetRightPanel.hpp"
#include "UI/widgets/WidgetSceneGraph.hpp"
#include "dialogs/DialogAddSceneObject.hpp"
#include "dialogs/DialogCreateMaterial.hpp"
#include "dialogs/DialogSceneExport.hpp"
#include "dialogs/DialogSceneRender.hpp"
#include "dialogs/DialogWaiting.hpp"
#include "core/SceneObject.hpp"
#include "core/AssetManager.hpp"
#include "core/io/Import.hpp"
#include "core/Lights.hpp"
#include "core/Materials.hpp"
#include "math/Transform.hpp"
#include "utils/ECS.hpp"
#include "utils/Tasks.hpp"

using namespace vengine;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QWidget *widgetViewport = initViewport();

    QHBoxLayout *layout_main = new QHBoxLayout();
    layout_main->addWidget(initLeftPanel());
    layout_main->addWidget(widgetViewport);
    layout_main->addWidget(initRightPanel());

    QWidget *widget_main = new QWidget();
    widget_main->setLayout(layout_main);

    m_scene = &m_engine->scene();

    createMenu();

    setCentralWidget(widget_main);
    resize(1600, 1100);
}

MainWindow::~MainWindow()
{
}

QWidget *MainWindow::initLeftPanel()
{
    m_sceneGraphWidget = new WidgetSceneGraph(this);
    m_sceneGraphWidget->setStyleSheet("background-color: rgba(240, 240, 240, 255);");
    connect(m_sceneGraphWidget, &QTreeWidget::itemSelectionChanged, this, &MainWindow::onSelectedSceneObjectChangedSlotUI);

    m_sceneGraphWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(
        m_sceneGraphWidget, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(onContextMenuSceneGraph(const QPoint &)));

    m_sceneGraphWidget->setHeaderLabel("Scene:");
    m_sceneGraphWidget->setFixedWidth(200);
    return m_sceneGraphWidget;
}

QWidget *MainWindow::initViewport()
{
    m_viewport = new VulkanViewportWindow();
    m_engine = m_viewport->engine();

    connect(m_viewport, &ViewportWindow::sceneObjectSelected, this, &MainWindow::onSelectedSceneObjectChangedSlot3DScene);
    connect(m_viewport, &ViewportWindow::initializationFinished, this, &MainWindow::onStartUpInitialization);

    return QWidget::createWindowContainer(m_viewport);
}

QWidget *MainWindow::initRightPanel()
{
    m_widgetRightPanel = new WidgetRightPanel(this, m_engine);
    connect(m_widgetRightPanel,
            &WidgetRightPanel::selectedSceneObjectNameChanged,
            this,
            &MainWindow::onSelectedSceneObjectNameChangedSlot);
    connect(m_viewport, &ViewportWindow::selectedObjectPositionChanged, m_widgetRightPanel, &WidgetRightPanel::onTransformChanged);

    m_widgetRightPanel->setFixedWidth(380);
    return m_widgetRightPanel;
}

void MainWindow::createMenu()
{
    QAction *actionImportModel = new QAction(tr("&Import a model"), this);
    actionImportModel->setStatusTip(tr("Import a model"));
    connect(actionImportModel, &QAction::triggered, this, &MainWindow::onImportModelSlot);
    QAction *actionImportColorTexture = new QAction(tr("&Import sRGB textures"), this);
    actionImportColorTexture->setStatusTip(tr("Import sRGB textures"));
    connect(actionImportColorTexture, &QAction::triggered, this, &MainWindow::onImportTextureSRGBSlot);
    QAction *actionImportOtherTexture = new QAction(tr("&Import linear textures"), this);
    actionImportOtherTexture->setStatusTip(tr("Import linear textures"));
    connect(actionImportOtherTexture, &QAction::triggered, this, &MainWindow::onImportTextureLinearSlot);
    QAction *actionImportEnvironmentMap = new QAction(tr("&Import environment map"), this);
    actionImportEnvironmentMap->setStatusTip(tr("Import environment map"));
    connect(actionImportEnvironmentMap, &QAction::triggered, this, &MainWindow::onImportEnvironmentMap);
    QAction *actionImportMaterial = new QAction(tr("&Import material"), this);
    actionImportMaterial->setStatusTip(tr("Import material"));
    connect(actionImportMaterial, &QAction::triggered, this, &MainWindow::onImportMaterial);
    QAction *actionImportMaterialZipStack = new QAction(tr("&Import material ZIP"), this);
    actionImportMaterialZipStack->setStatusTip(tr("Import material ZIP"));
    connect(actionImportMaterialZipStack, &QAction::triggered, this, &MainWindow::onImportMaterialZipStackSlot);
    QAction *actionImportScene = new QAction(tr("&Import scene"), this);
    actionImportScene->setStatusTip(tr("Import scene"));
    connect(actionImportScene, &QAction::triggered, this, &MainWindow::onImportScene);

    QAction *actionCreateMaterial = new QAction(tr("&Create a material"), this);
    actionCreateMaterial->setStatusTip(tr("Create a material"));
    connect(actionCreateMaterial, &QAction::triggered, this, &MainWindow::onAddMaterialSlot);

    QAction *actionRender = new QAction(tr("&Render scene"), this);
    actionRender->setStatusTip(tr("Render scene"));
    connect(actionRender, &QAction::triggered, this, &MainWindow::onRenderSceneSlot);

    QAction *actionExport = new QAction(tr("&Export scene"), this);
    actionExport->setStatusTip(tr("Export scene"));
    connect(actionExport, &QAction::triggered, this, &MainWindow::onExportSceneSlot);

    QMenu *m_menuImport = menuBar()->addMenu(tr("&Import"));
    m_menuImport->addAction(actionImportModel);
    m_menuImport->addAction(actionImportColorTexture);
    m_menuImport->addAction(actionImportOtherTexture);
    m_menuImport->addAction(actionImportEnvironmentMap);
    m_menuImport->addAction(actionImportMaterial);
    m_menuImport->addAction(actionImportMaterialZipStack);
    m_menuImport->addAction(actionImportScene);
    QMenu *m_menuAdd = menuBar()->addMenu(tr("&Create"));
    m_menuAdd->addAction(actionCreateMaterial);
    QMenu *m_menuRender = menuBar()->addMenu(tr("&Render"));
    m_menuRender->addAction(actionRender);
    QMenu *m_menuExport = menuBar()->addMenu(tr("&Export"));
    m_menuExport->addAction(actionExport);
}

std::pair<QTreeWidgetItem *, std::shared_ptr<SceneObject>> MainWindow::createEmptySceneObject(std::string name,
                                                                                              const Transform &transform,
                                                                                              QTreeWidgetItem *parent)
{
    /* If parent is null, add at root */
    if (parent == nullptr) {
        std::shared_ptr<SceneObject> newObject = m_scene->addSceneObject(name, transform);
        QTreeWidgetItem *widgetItem = m_sceneGraphWidget->createTreeWidgetItem(newObject);

        m_sceneGraphWidget->addTopLevelItem(widgetItem);

        return {widgetItem, newObject};
    } else {
        std::shared_ptr<SceneObject> parentObject = WidgetSceneGraph::getSceneObject(parent);

        std::shared_ptr<SceneObject> newObject = m_scene->addSceneObject(name, parentObject, transform);
        QTreeWidgetItem *widgetItem = m_sceneGraphWidget->createTreeWidgetItem(newObject);

        parent->addChild(widgetItem);

        return {widgetItem, newObject};
    }
}

void MainWindow::selectObject(QTreeWidgetItem *selectedItem)
{
    QTreeWidgetItem *previouslySelected = m_sceneGraphWidget->getPreviouslySelectedItem();
    /* If it's the previously selected item, return */
    if (selectedItem == previouslySelected)
        return;

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
    m_engine->renderer().setSelectedObject(sceneObject);

    m_widgetRightPanel->setSelectedObject(sceneObject);

    m_sceneGraphWidget->blockSignals(false);
}

void MainWindow::removeObjectFromScene(QTreeWidgetItem *treeItem)
{
    /* Get corresponding scene object */
    std::shared_ptr<SceneObject> selectedObject = WidgetSceneGraph::getSceneObject(treeItem);

    /* Remove scene object from scene */
    m_scene->removeSceneObject(selectedObject);

    /* Set selected to null */
    m_engine->renderer().setSelectedObject(nullptr);

    /* Remove from scene graph UI */
    m_sceneGraphWidget->removeItem(treeItem);
}

void MainWindow::addSceneObjectModel(QTreeWidgetItem *parentItem, std::string modelName, std::optional<std::string> overrideMaterial)
{
    auto &instanceModels = AssetManager::getInstance().modelsMap();
    if (!instanceModels.isPresent(modelName)) {
        debug_tools::ConsoleWarning("MainWindow::addSceneObjectModel(): Model " + modelName + " is not imported");
        return;
    }
    auto model = instanceModels.get(modelName);
    Tree<Model3D::Model3DNode> &modelData = model->data();

    auto &instanceMaterials = AssetManager::getInstance().materialsMap();
    std::shared_ptr<Material> defaultMat = instanceMaterials.get("defaultMaterial");
    std::shared_ptr<Material> overrideMat = (overrideMaterial.has_value() ? instanceMaterials.get(overrideMaterial.value()) : nullptr);

    std::function<void(QTreeWidgetItem *, Tree<Model3D::Model3DNode> &)> addModelF = [&](QTreeWidgetItem *parent,
                                                                                         Tree<Model3D::Model3DNode> &modelData) {
        auto &modelNode = modelData.data();
        for (uint32_t i = 0; i < modelNode.meshes.size(); i++) {
            auto &mesh = modelNode.meshes[i];
            auto mat = modelNode.materials[i];

            auto child = createEmptySceneObject(mesh->name(), modelNode.transform, parent);

            child.second->add<ComponentMesh>().mesh = mesh;
            if (overrideMat != nullptr) {
                child.second->add<ComponentMaterial>().material = overrideMat;
            } else if (mat != nullptr) {
                child.second->add<ComponentMaterial>().material = mat;
            } else {
                child.second->add<ComponentMaterial>().material = defaultMat;
            }
        }

        if (modelData.size() > 0) {
            auto so = createEmptySceneObject(modelNode.name, modelNode.transform, parent);
            for (uint32_t i = 0; i < modelData.size(); i++) {
                addModelF(so.first, modelData.child(i));
            }
        }
    };

    addModelF(parentItem, modelData);
}

void MainWindow::addImportedSceneObject(const Tree<ImportedSceneObject> &scene, QTreeWidgetItem *parentItem)
{
    auto &instanceModels = AssetManager::getInstance().modelsMap();
    auto &instanceMaterials = AssetManager::getInstance().materialsMap();
    auto &instanceLightMaterials = AssetManager::getInstance().lightMaterialsMap();

    const ImportedSceneObject &object = scene.data();

    /* Create new scene object */
    auto newSceneObject = createEmptySceneObject(object.name, object.transform, parentItem);

    /* Add mesh component */
    if (object.mesh.has_value()) {
        if (object.mesh->submesh.empty()) {
            /* if submesh is not defined, assign the whole model under the parent item */
            addSceneObjectModel(parentItem, object.mesh->path, std::nullopt);
            return;
        } else {
            /* Else, assign the corresponding submesh to the object */
            auto model3D = instanceModels.get(object.mesh->path);
            auto mesh = model3D->mesh(object.mesh->submesh);
            newSceneObject.second->add<ComponentMesh>().mesh = mesh;
        }
    }

    /* Add material component */
    if (object.material.has_value()) {
        auto mat = instanceMaterials.get(object.material->name);
        newSceneObject.second->add<ComponentMaterial>().material = mat;
    }

    /* Add light component */
    if (object.light.has_value()) {
        auto lightMat = instanceLightMaterials.get(object.light->lightMaterial);
        newSceneObject.second->add<ComponentLight>().light = std::make_shared<Light>("light", LightType::POINT_LIGHT, lightMat);
    }

    /* Add children */
    for (uint32_t c = 0; c < scene.size(); c++) {
        addImportedSceneObject(scene.child(c), newSceneObject.first);
    }

    return;
}

bool MainWindow::event(QEvent *event)
{
    if (event->type() == QEvent::WindowActivate) {
        m_viewport->windowActicated(true);
    }
    if (event->type() == QEvent::WindowDeactivate) {
        m_viewport->windowActicated(false);
    }

    return QMainWindow::event(event);
}

void MainWindow::onImportModelSlot()
{
    QString filename = QFileDialog::getOpenFileName(
        this, tr("Import model"), "./assets/models", tr("Model (*.obj *.glb *.gltf *.fbx);;All Files (*)"));

    if (filename == "")
        return;

    struct ImportFunct {
        ImportFunct(Engine *e, std::string f)
            : engine(e)
            , filename(f){};
        Engine *engine;
        std::string filename;
        bool operator()(float &)
        {
            bool ret = engine->importModel(filename, true) != nullptr;
            if (ret) {
                debug_tools::ConsoleInfo("Model imported");
            }
            return ret;
        }
    };
    Task task(ImportFunct(m_engine, filename.toStdString()));

    DialogWaiting *waiting = new DialogWaiting(nullptr, "Importing...", &task);
    waiting->exec();

    delete waiting;
}

void MainWindow::onImportTextureSRGBSlot()
{
    QStringList filenames =
        QFileDialog::getOpenFileNames(this, tr("Import textures"), "./assets", tr("Textures (*.tga, *.png);;All Files (*)"));

    if (filenames.length() == 0)
        return;

    struct ImportFunct {
        ImportFunct(Textures &t, QStringList &fs)
            : textures(t)
            , filenames(fs){};
        Textures &textures;
        QStringList filenames;

        bool operator()(float &progress)
        {
            bool success = true;
            for (uint32_t t = 0; t < filenames.length(); t++) {
                const auto &texture = filenames[t];
                auto tex = textures.createTexture(texture.toStdString(), ColorSpace::sRGB);
                if (tex) {
                    debug_tools::ConsoleInfo("Texture: " + texture.toStdString() + " imported");
                } else {
                    success = false;
                }
                progress = static_cast<float>(t) / static_cast<float>(filenames.length());
            }
            return success;
        }
    };
    Task task(ImportFunct(m_engine->textures(), filenames));

    DialogWaiting *waiting = new DialogWaiting(nullptr, "Importing...", &task);
    waiting->exec();

    delete waiting;
}

void MainWindow::onImportTextureLinearSlot()
{
    QStringList filenames =
        QFileDialog::getOpenFileNames(this, tr("Import textures"), "./assets", tr("Textures (*.tga, *.png);;All Files (*)"));

    if (filenames.length() == 0)
        return;

    struct ImportFunct {
        ImportFunct(Textures &t, QStringList &fs)
            : textures(t)
            , filenames(fs){};
        Textures &textures;
        QStringList filenames;

        bool operator()(float &progress)
        {
            bool success = true;
            for (uint32_t t = 0; t < filenames.length(); t++) {
                const auto &texture = filenames[t];
                auto tex = textures.createTexture(texture.toStdString(), ColorSpace::LINEAR);
                if (tex) {
                    debug_tools::ConsoleInfo("Texture: " + texture.toStdString() + " imported");
                } else {
                    success = false;
                }
                progress = static_cast<float>(t) / static_cast<float>(filenames.length());
            }
            return success;
        }
    };
    Task task(ImportFunct(m_engine->textures(), filenames));

    DialogWaiting *waiting = new DialogWaiting(nullptr, "Importing...", &task);
    waiting->exec();

    delete waiting;
}

void MainWindow::onImportEnvironmentMap()
{
    QString filename =
        QFileDialog::getOpenFileName(this, tr("Import HDR texture"), "./assets/HDR/", tr("Model (*.hdr);;All Files (*)"));

    if (filename == "")
        return;

    struct ImportFunct {
        ImportFunct(Engine *e, std::string f)
            : engine(e)
            , filename(f){};
        Engine *engine;
        std::string filename;
        bool operator()(float &)
        {
            auto envMap = engine->importEnvironmentMap(filename);
            if (envMap) {
                debug_tools::ConsoleInfo("Environment map: " + filename + " imported");
                return true;
            }
            return false;
        }
    };
    Task task(ImportFunct(m_engine, filename.toStdString()));

    DialogWaiting *waiting = new DialogWaiting(nullptr, "Importing...", &task);
    waiting->exec();

    m_widgetRightPanel->getEnvironmentWidget()->updateMaps();

    delete waiting;
}

void MainWindow::onImportMaterial()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Choose directory", "./assets/materials/", QFileDialog::ShowDirsOnly);
    if (dir == "")
        return;

    std::string materialName = dir.split('/').back().toStdString();
    std::string dirStd = dir.toStdString();

    struct ImportFunct {
        ImportFunct(Engine *e, std::string d, std::string n)
            : engine(e)
            , dir(d)
            , materialName(n){};
        Engine *engine;
        std::string dir;
        std::string materialName;
        bool operator()(float &)
        {
            auto material = engine->materials().createMaterialFromDisk(materialName, dir, engine->textures());
            if (material) {
                debug_tools::ConsoleInfo("Material: " + materialName + " has been created");
                return true;
            } else {
                return false;
            }
        }
    };
    Task task(ImportFunct(m_engine, dirStd, materialName));

    DialogWaiting *waiting = new DialogWaiting(nullptr, "Importing...", &task);
    waiting->exec();

    if (task.success) {
        m_widgetRightPanel->updateAvailableMaterials();
    }

    delete waiting;
}

void MainWindow::onImportMaterialZipStackSlot()
{
    QString filename =
        QFileDialog::getOpenFileName(this, tr("Import zip stack material"), "./assets/materials/", tr("Model (*.zip);;All Files (*)"));

    if (filename == "")
        return;

    std::string materialName = filename.split('/').back().toStdString();

    struct ImportFunct {
        ImportFunct(Engine *e, std::string f, std::string n)
            : engine(e)
            , filename(f)
            , materialName(n){};
        Engine *engine;
        std::string filename;
        std::string materialName;
        bool operator()(float &)
        {
            auto material = engine->materials().createZipMaterial(materialName, filename, engine->textures());
            if (material) {
                debug_tools::ConsoleInfo("Material: " + materialName + " has been created");
                return true;
            } else {
                return false;
            }
        }
    };
    Task task(ImportFunct(m_engine, filename.toStdString(), materialName));

    DialogWaiting *waiting = new DialogWaiting(nullptr, "Importing...", &task);
    waiting->exec();

    if (task.success) {
        m_widgetRightPanel->updateAvailableMaterials();
    }

    delete waiting;
}

void MainWindow::onImportScene()
{
    /* Select json file */
    QString sceneFile =
        QFileDialog::getOpenFileName(this, tr("Import scene"), "./assets/scenes/", tr("Model (*.json);;All Files (*)"));

    if (sceneFile == "")
        return;

    /* Parse json file */
    ImportedCamera camera;
    Tree<ImportedSceneObject> scene;
    std::vector<std::string> models;
    std::vector<ImportedMaterial> materials;
    std::vector<ImportedLightMaterial> lightMaterials;
    ImportedEnvironment env;
    std::string sceneFolder;
    try {
        sceneFolder = importScene(sceneFile.toStdString(), camera, scene, models, materials, lightMaterials, env);
    } catch (std::runtime_error &e) {
        debug_tools::ConsoleWarning("Unable to open scene file: " + std::string(e.what()));
        return;
    }

    m_engine->stop();
    m_engine->waitIdle();

    /* Set camera */
    {
        auto newCamera = std::make_shared<PerspectiveCamera>();
        Transform &cameraTransform = newCamera->transform();
        cameraTransform.position() = camera.position;
        cameraTransform.setRotation(glm::normalize(camera.target - camera.position), camera.up);

        newCamera->fov() = camera.fov;
        newCamera->setWindowSize(m_viewport->size().width(), m_viewport->size().height());

        m_scene->setCamera(newCamera);
        m_widgetRightPanel->getEnvironmentWidget()->setCamera(newCamera);
    }

    /* Delete previous assets */
    AssetManager::getInstance().materialsMap().reset();
    AssetManager::getInstance().lightMaterialsMap().reset();
    AssetManager::getInstance().modelsMap().reset();

    /* Remove old scene */
    m_sceneGraphWidget->blockSignals(true);
    while (m_sceneGraphWidget->topLevelItemCount() != 0) {
        removeObjectFromScene(m_sceneGraphWidget->topLevelItem(0));
    }
    m_sceneGraphWidget->setPreviouslySelectedItem(nullptr);
    m_sceneGraphWidget->blockSignals(false);

    /* Import models */
    debug_tools::ConsoleInfo("Importing models...");
    for (auto &itr : models) {
        m_engine->importModel(itr, true);
    }

    /* Import materials */
    debug_tools::ConsoleInfo("Importing materials...");
    m_engine->materials().createImportedMaterials(materials, m_engine->textures());

    /* Import light materials */
    for (auto &itr : lightMaterials) {
        AssetManager::getInstance().lightMaterialsMap().add(std::make_shared<LightMaterial>(itr.name, itr.color, itr.intensity));
    }

    /* Create new scene */
    debug_tools::ConsoleInfo("Creating new scene...");
    for (uint32_t c = 0; c < scene.size(); c++) {
        addImportedSceneObject(scene.child(c), nullptr);
    }

    /* Create and set the environment */
    if (!env.path.empty()) {
        auto envMap = m_engine->importEnvironmentMap(sceneFolder + env.path);
        if (envMap) {
            debug_tools::ConsoleInfo("Environment map: " + sceneFolder + env.path + " set");

            auto &materialsSkybox = AssetManager::getInstance().materialsSkyboxMap();
            auto material = materialsSkybox.get("skybox");
            material->setMap(envMap);
        }
    }
    m_widgetRightPanel->getEnvironmentWidget()->setBackgroundColor(env.backgroundColor);
    m_widgetRightPanel->getEnvironmentWidget()->updateMaps();
    m_widgetRightPanel->getEnvironmentWidget()->setEnvironmentType(env.environmentType);

    m_engine->start();
    debug_tools::ConsoleInfo(sceneFile.toStdString() + " imported");
}

void MainWindow::onAddEmptyObjectSlot()
{
    /* Get currently selected tree item */
    QTreeWidgetItem *selectedItem = m_sceneGraphWidget->currentItem();

    m_engine->stop();
    m_engine->waitIdle();

    /* Create parent a oject for the mesh model */
    /* TODO set the name in some other way */
    auto parent = createEmptySceneObject("New object (" + std::to_string(m_nObjects++) + ")", Transform(), selectedItem);

    m_engine->start();
}

void MainWindow::onAddSceneObjectSlot()
{
    /* Get currently selected tree item */
    QTreeWidgetItem *selectedItem = m_sceneGraphWidget->currentItem();

    DialogAddSceneObject *dialog =
        new DialogAddSceneObject(nullptr, "Add an object to the scene", getImportedModels(), getCreatedMaterials());
    dialog->exec();
    std::string selectedModel = dialog->getSelectedModel();
    if (selectedModel == "") {
        return;
    }

    m_engine->stop();
    m_engine->waitIdle();

    addSceneObjectModel(selectedItem, selectedModel, dialog->getSelectedOverrideMaterial());

    m_engine->start();
}

void MainWindow::onRemoveSceneObjectSlot()
{
    /* Get the currently selected tree item */
    QTreeWidgetItem *selectedItem = m_sceneGraphWidget->currentItem();

    if (selectedItem == nullptr)
        return;

    m_engine->stop();
    m_engine->waitIdle();

    removeObjectFromScene(selectedItem);

    m_engine->start();
}

void MainWindow::onAddMaterialSlot()
{
    DialogCreateMaterial *dialog = new DialogCreateMaterial(nullptr, "Create a material", getImportedModels());
    dialog->exec();

    if (dialog->m_selectedMaterialType == MaterialType::MATERIAL_NOT_SET)
        return;

    std::string materialName = dialog->m_selectedName.toStdString();
    if (materialName == "") {
        debug_tools::ConsoleWarning("Material name can't be empty");
        return;
    }

    auto material = m_engine->materials().createMaterial(materialName, dialog->m_selectedMaterialType);
    if (material == nullptr) {
        debug_tools::ConsoleWarning("Failed to create material");
    } else {
        m_widgetRightPanel->updateAvailableMaterials();
    }
}

void MainWindow::onAddPointLightSlot()
{
    /* Get currently selected tree item */
    QTreeWidgetItem *selectedItem = m_sceneGraphWidget->currentItem();

    m_engine->stop();
    m_engine->waitIdle();

    /* Create new empty item under the selected item */
    auto newObject = createEmptySceneObject("Point light", Transform(), selectedItem);

    /* Add light component */
    auto &lightMaterials = AssetManager::getInstance().lightMaterialsMap();
    if (!lightMaterials.isPresent("DefaultPointLightMaterial")) {
        lightMaterials.add(std::make_shared<LightMaterial>("DefaultPointLightMaterial", glm::vec3(1, 1, 1), 1));
    }
    auto lightMat = lightMaterials.get("DefaultPointLightMaterial");

    newObject.second->add<ComponentLight>().light =
        std::make_shared<Light>(newObject.second->m_name, LightType::POINT_LIGHT, lightMat);

    m_engine->start();
}

void MainWindow::onRenderSceneSlot()
{
    auto &RTrenderer = m_engine->renderer().rendererRayTracing();

    if (!RTrenderer.isRTEnabled()) {
        int ret = QMessageBox::warning(
            this,
            tr("Error"),
            tr("GPU Ray tracing has not been initialized. Most likely, no ray tracing capable GPU has been found"),
            QMessageBox::Ok);

        return;
    }

    DialogSceneRender *dialog = new DialogSceneRender(nullptr, RTrenderer.renderInfo());
    dialog->exec();

    std::string filename = dialog->getRenderOutputFileName();
    if (filename == "") {
        return;
    }

    RTrenderer.renderInfo().samples = dialog->getSamples();
    RTrenderer.renderInfo().depth = dialog->getDepth();
    RTrenderer.renderInfo().batchSize = dialog->getBatchSize();
    RTrenderer.renderInfo().filename = dialog->getRenderOutputFileName();
    RTrenderer.renderInfo().fileType = dialog->getRenderOutputFileType();
    RTrenderer.renderInfo().width = dialog->getResolutionWidth();
    RTrenderer.renderInfo().height = dialog->getResolutionHeight();
    delete dialog;

    struct RTRenderTask : public Task {
        RendererRayTracing &renderer;

        RTRenderTask(RendererRayTracing &r, Scene &s)
            : renderer(r)
        {
            function = Funct(renderer, s);
        };

        struct Funct {
            RendererRayTracing &renderer;
            Scene &scene;

            Funct(RendererRayTracing &r, Scene &s)
                : renderer(r)
                , scene(s){};
            bool operator()(float &)
            {
                renderer.render(scene);
                return true;
            }
        };

        float getProgress() const override { return renderer.renderProgress(); }
    };
    auto task = RTRenderTask(RTrenderer, *m_scene);

    /* Wait for the render loop to idle */
    m_engine->stop();
    m_engine->waitIdle();

    /* Spawn rendering task */
    DialogWaiting *waiting = new DialogWaiting(nullptr, "Rendering...", &task);
    waiting->exec();

    m_engine->start();

    delete waiting;
}

void MainWindow::onExportSceneSlot()
{
    DialogSceneExport *dialog = new DialogSceneExport(nullptr);
    dialog->exec();

    std::string folderName = dialog->getDestinationFolderName();
    if (folderName == "")
        return;

    ExportRenderParams params;
    params.name = dialog->getDestinationFolderName();
    params.backgroundColor = m_scene->getBackgroundColor();
    params.environmentType = static_cast<int>(m_scene->getEnvironmentType());

    m_scene->exportScene(params);

    debug_tools::ConsoleInfo("Scene exported: " + folderName);

    delete dialog;
}

void MainWindow::onDuplicateSceneObjectSlot()
{
    /* Get currently selected tree item */
    QTreeWidgetItem *selectedItem = m_sceneGraphWidget->currentItem();

    std::function<void(QTreeWidgetItem *, QTreeWidgetItem *)> duplicate;
    duplicate = [&](QTreeWidgetItem *duplicatedItem, QTreeWidgetItem *parent) {
        std::shared_ptr<SceneObject> so = m_sceneGraphWidget->getSceneObject(duplicatedItem);
        auto newObject = createEmptySceneObject(so->m_name + " (D)", so->m_localTransform, parent);

        if (so->has<ComponentLight>()) {
            auto l = so->get<ComponentLight>().light;
            newObject.second->add<ComponentLight>().light =
                std::make_shared<Light>(newObject.second->m_name, l->type, l->lightMaterial);
        }
        if (so->has<ComponentMesh>()) {
            newObject.second->add<ComponentMesh>().mesh = so->get<ComponentMesh>().mesh;
        }
        if (so->has<ComponentMaterial>()) {
            newObject.second->add<ComponentMaterial>().material = so->get<ComponentMaterial>().material;
        }

        for (int c = 0; c < duplicatedItem->childCount(); c++) {
            duplicate(duplicatedItem->child(c), newObject.first);
        }
    };

    m_engine->stop();
    m_engine->waitIdle();

    duplicate(selectedItem, selectedItem->parent());

    m_engine->start();
}

void MainWindow::onSelectedSceneObjectChangedSlotUI()
{
    /* Get currently selected object, and the corresponding SceneObject */
    QTreeWidgetItem *selectedItem = m_sceneGraphWidget->currentItem();
    /* When all items have been removed, this function is called with a null object selected */

    if (selectedItem == nullptr) {
        return;
    }

    selectObject(selectedItem);
}

void MainWindow::onSelectedSceneObjectChangedSlot3DScene(std::shared_ptr<SceneObject> object)
{
    QTreeWidgetItem *treeItem = m_sceneGraphWidget->getTreeWidgetItem(object);
    if (treeItem == nullptr)
        return;

    selectObject(treeItem);
}

void MainWindow::onSelectedSceneObjectNameChangedSlot(QString newName)
{
    QTreeWidgetItem *selectedItem = m_sceneGraphWidget->currentItem();
    std::shared_ptr<SceneObject> object = WidgetSceneGraph::getSceneObject(selectedItem);

    object->m_name = newName.toStdString();
    selectedItem->setText(0, newName);
}

void MainWindow::onContextMenuSceneGraph(const QPoint &pos)
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
    QAction *action5 = nullptr;
    if (index.isValid()) {
        action5 = new QAction("Duplicate", this);
        connect(action5, SIGNAL(triggered()), this, SLOT(onDuplicateSceneObjectSlot()));
        contextMenu.addAction(action5);
    }

    contextMenu.exec(m_sceneGraphWidget->mapToGlobal(pos));
    if (action5 != nullptr)
        delete action5;
}

void MainWindow::onStartUpInitialization()
{
    std::string assetName = "assets/models/DamagedHelmet.gltf";
    m_engine->importModel(assetName, true);

    auto &instanceModels = AssetManager::getInstance().modelsMap();
    auto &instanceMaterials = AssetManager::getInstance().materialsMap();
    auto &instanceLightMaterials = AssetManager::getInstance().lightMaterialsMap();

    /* Create a plane scene with the assetName on top */
    try {
        auto matDef = instanceMaterials.get("defaultMaterial");
        auto plane = instanceModels.get("assets/models/plane.obj");

        auto o2 = createEmptySceneObject("plane", Transform({0, -1, 0}, {10, 10, 10}), nullptr);
        o2.second->add<ComponentMesh>().mesh = plane->mesh("Plane");
        o2.second->add<ComponentMaterial>().material = matDef;

        addSceneObjectModel(NULL, assetName, std::nullopt);

    } catch (std::exception &e) {
        debug_tools::ConsoleCritical("Failed to setup initialization scene: " + std::string(e.what()));
    }
}
