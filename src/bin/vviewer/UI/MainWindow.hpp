#ifndef __MainWindow_hpp__
#define __MainWindow_hpp__

#include <qmainwindow.h>
#include <qwidget.h>
#include <qslider.h>
#include <qlistwidget.h>
#include <qlayout.h>
#include <qtreewidget.h>

#include "widgets/WidgetName.hpp"
#include "widgets/WidgetTransform.hpp"
#include "widgets/WidgetMeshModel.hpp"
#include "widgets/WidgetMaterial.hpp"
#include "widgets/WidgetEnvironment.hpp"
#include "widgets/WidgetLight.hpp"
#include "widgets/WidgetMeshModel.hpp"
#include "widgets/WidgetComponent.hpp"
#include "widgets/WidgetRightPanel.hpp"
#include "widgets/WidgetSceneGraph.hpp"

#include "core/Scene.hpp"
#include "core/Import.hpp"
#include "vulkan/VulkanWindow.hpp"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent);
    virtual ~MainWindow();

private:
    /* UI vulkan */
    QVulkanInstance * m_vulkanInstance;
    VulkanWindow * m_vulkanWindow;

    /* For naming new objects */
    int m_nObjects = 0;

    Scene* m_scene = nullptr;

    WidgetSceneGraph * m_sceneGraphWidget = nullptr;
    WidgetRightPanel * m_widgetRightPanel = nullptr;

    QWidget * initLeftPanel();
    QWidget * initVulkanWindowWidget();
    QWidget * initRightPanel();
    void createMenu();

    /**
     * @brief Create an Empty Scene Object with name and transform under a parent. If parent is null add at root
     * 
     * @param name 
     * @param transform 
     * @param parent 
     * @return The UI tree widget item, and the actual scene object
     */
    std::pair<QTreeWidgetItem*, std::shared_ptr<SceneObject>> createEmptySceneObject(std::string name, const Transform& transform, QTreeWidgetItem* parent);

    /**
     * @brief Change UI selection to the selectedItem 
     * 
     * @param selectedItem 
     */
    void selectObject(QTreeWidgetItem* selectedItem);

    /**
     * @brief Remove an item from the scene
     * 
     * @param treeItem 
     */
    void removeObjectFromScene(QTreeWidgetItem* treeItem);

    /**
     * @brief Add all the meshes of the modelName under the parentItem as new scene objects with the requested material
     * 
     * @param parentItem 
     * @param modelName 
     * @param material 
     */
    void addSceneObjectMeshes(QTreeWidgetItem * parentItem, std::string modelName, std::string material);

    void addImportedSceneObject(const ImportedSceneObject& object, 
            const std::unordered_map<std::string, ImportedSceneMaterial>& materials,
            const std::unordered_map<std::string, ImportedSceneLightMaterial>& lights,
            QTreeWidgetItem * parentItem, std::string sceneFolder);


    bool event(QEvent* event) override;

private slots:
    /* Import a model */
    void onImportModelSlot();
    /* Import a color texture */
    void onImportTextureColorSlot();
    /* Import a texture */
    void onImportTextureOtherSlot();
    /* Import an hdr texture */
    void onImportTextureHDRSlot();
    /* Import an environment map */
    void onImportEnvironmentMap();
    /* Import a material */
    void onImportMaterial();
    /* Import a material from a zip stack */
    void onImportMaterialZipStackSlot();
    /* Import a scene */
    void onImportScene();

    /* Add an empty object in the scene */
    void onAddEmptyObjectSlot();
    /* Add an object in the scene */
    void onAddSceneObjectSlot();
    /* Remove the selected object from the scene */
    void onRemoveSceneObjectSlot();
    /* Create a material */
    void onAddMaterialSlot();
    /* Add a point light as a child of the currently selected node */
    void onAddPointLightSlot();
    /* Render scene */
    void onRenderSceneSlot();
    /* Export scene */
    void onExportSceneSlot();
    /* Duplicate scene object */
    void onDuplicateSceneObjectSlot();

    /* Currently selected item in the scene changed from UI */
    void onSelectedSceneObjectChangedSlotUI();
    /* Currently selected item in the scene changed from the 3d scene */
    void onSelectedSceneObjectChangedSlot3DScene(std::shared_ptr<SceneObject> object);
    /* Currently selected item's name in the scene changed */
    void onSelectedSceneObjectNameChangedSlot(QString newName);

    /* Show context menu for scene graph */
    void onContextMenuSceneGraph(const QPoint& pos);

    /* Start up scene initialization */
    void onStartUpInitialization();
};

#endif