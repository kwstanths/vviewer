#ifndef __MainWindow_hpp__
#define __MainWindow_hpp__

#include <qmainwindow.h>
#include <qwidget.h>
#include <qslider.h>
#include <qlistwidget.h>
#include <qlayout.h>
#include <qtreewidget.h>

#include "WidgetName.hpp"
#include "WidgetTransform.hpp"
#include "WidgetMeshModel.hpp"
#include "WidgetMaterial.hpp"
#include "WidgetEnvironment.hpp"
#include "WidgetPointLight.hpp"
#include "WidgetMeshModel.hpp"

#include "core/Scene.hpp"
#include "core/Import.hpp"
#include "vulkan/VulkanWindow.hpp"

Q_DECLARE_METATYPE(std::shared_ptr<SceneObject>)

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent);
    virtual ~MainWindow();

private:
    /* UI */
    QVBoxLayout * m_layoutControls;
    /* UI vulkan */
    QVulkanInstance * m_vulkanInstance;
    VulkanWindow * m_vulkanWindow;

    /* For naming new objects */
    int m_nObjects = 0;
    /* Holds a pointer to the scene */
    Scene* m_scene = nullptr;
    /* Holds a pointer to the scene graph widget */
    QTreeWidget* m_sceneGraphWidget = nullptr;
    /* Previously selected item */
    QTreeWidgetItem* m_selectedPreviousWidgetItem = nullptr;
    std::unordered_map<ID, QTreeWidgetItem*> m_treeWidgetItems;

    /* Widgets that appear on the controls on the right panel */
    WidgetName * m_selectedObjectWidgetName = nullptr;
    WidgetTransform * m_selectedObjectWidgetTransform = nullptr;
    WidgetMaterial * m_selectedObjectWidgetMaterial = nullptr;
    WidgetPointLight * m_selectedObjectWidgetPointLight = nullptr;
    WidgetMeshModel * m_selectedObjectWidgetMeshModel = nullptr;
    WidgetEnvironment * m_widgetEnvironment = nullptr;

    QWidget * initLeftPanel();
    QWidget * initVulkanWindowWidget();
    QWidget * initControlsWidget();
    void createMenu();

    QTreeWidgetItem* createTreeWidgetItem(std::shared_ptr<SceneObject> object);
    std::shared_ptr<SceneObject> getSceneObject(QTreeWidgetItem* item) const;

    /**
     * @brief Create an Empty Scene Object with name and transform under parent. If parent is null add at root
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
    void clearControlsUI();

    bool addImportedSceneObject(const ImportedSceneObject& object, 
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

    /* Add an object in the scene at root */
    void onAddSceneObjectRootSlot();
    /* Add an object in the scene as a child to the currently selected node */
    void onAddSceneObjectSlot();
    /* Remove the selected object from the scene */
    void onRemoveSceneObjectSlot();
    /* Create a material */
    void onAddMaterialSlot();
    /* Add a point light at root */
    void onAddPointLightRootSlot();
    /* Add a point light as a child of the currently selected node */
    void onAddPointLightSlot();
    /* Render scene */
    void onRenderSceneSlot();
    /* Export scene */
    void onExportSceneSlot();
    
    /* Currently selected item in the scene changed from UI */
    void onSelectedSceneObjectChangedSlotUI();
    /* Currently selected item in the scene changed from the 3d scene */
    void onSelectedSceneObjectChangedSlot3DScene(std::shared_ptr<SceneObject> object);
    /* Currently selected item's name in the scene changed */
    void onSelectedSceneObjectNameChangedSlot();

    /* Show context menu for scene graph */
    void onContextMenuSceneGraph(const QPoint& pos);

    /* Start up scene initialization */
    void onStartUpInitialization();
};

#endif