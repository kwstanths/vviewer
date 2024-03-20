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
#include "widgets/WidgetModel3D.hpp"
#include "widgets/WidgetMaterial.hpp"
#include "widgets/WidgetEnvironment.hpp"
#include "widgets/WidgetLight.hpp"
#include "widgets/WidgetComponent.hpp"
#include "widgets/WidgetRightPanel.hpp"
#include "widgets/WidgetSceneGraph.hpp"

#include "vengine/core/Scene.hpp"
#include "vengine/core/io/Import.hpp"
#include "vengine/utils/Tree.hpp"

#include "ViewportWindow.hpp"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent);
    virtual ~MainWindow();

private:
    /* For naming new objects */
    int m_nObjects = 0;

    vengine::Scene *m_scene = nullptr;
    vengine::Engine *m_engine = nullptr;

    WidgetSceneGraph *m_sceneGraphWidget = nullptr;
    WidgetRightPanel *m_widgetRightPanel = nullptr;
    ViewportWindow *m_viewport = nullptr;

    QWidget *initLeftPanel();
    QWidget *initViewport();
    QWidget *initRightPanel();
    void createMenu();

    /**
     * @brief Create an Empty Scene Object with name and transform under a parent. If parent is null add at root
     *
     * @param name
     * @param transform
     * @param parent
     * @return The UI tree widget item, and the actual scene object
     */
    std::pair<QTreeWidgetItem *, vengine::SceneObject *> createEmptySceneObject(std::string name,
                                                                                const vengine::Transform &transform,
                                                                                QTreeWidgetItem *parent);

    /**
     * @brief Change UI selection to the selectedItem
     *
     * @param selectedItem
     */
    void selectObject(QTreeWidgetItem *selectedItem);

    /**
     * @brief Remove an item from the scene
     *
     * @param treeItem
     */
    void removeObjectFromScene(QTreeWidgetItem *treeItem);

    void addSceneObjectModel(QTreeWidgetItem *parentItem,
                             std::string modelName,
                             std::optional<vengine::Transform> overrideRootTransform = std::nullopt,
                             std::optional<std::string> overrideMaterial = std::nullopt);

    void addImportedSceneObject(const vengine::Tree<vengine::ImportedSceneObject> &object, QTreeWidgetItem *parentItem);

    bool event(QEvent *event) override;

private Q_SLOTS:
    /* Import a model */
    void onImportModelSlot();
    /* Import a color texture */
    void onImportTextureSRGBSlot();
    /* Import a texture */
    void onImportTextureLinearSlot();
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
    /* Add a point light as a child of the currently selected node */
    void onAddPointLightSlot();
    /* Add a directional light as a child of the currently selected node */
    void onAddDirectionalLightSlot();
    /* Add a mesh light as a child of the currently selected node */
    void onAddMeshLightSlot();
    /* Create a material */
    void onCreateMaterialSlot();
    /* Create a light */
    void onCreateLightSlot();
    /* Render scene */
    void onRenderSceneSlot();
    /* Export scene */
    void onExportSceneSlot();
    /* Duplicate scene object */
    void onDuplicateSceneObjectSlot();

    /* Currently selected item in the scene changed from UI */
    void onSelectedSceneObjectChangedSlotUI();
    /* Currently selected item in the scene changed from the 3d scene */
    void onSelectedSceneObjectChangedSlot3DScene(vengine::SceneObject *object);
    /* Currently selected item's name in the scene changed */
    void onSelectedSceneObjectNameChangedSlot(QString newName);
    /* Currently selected item's active property changed */
    void onSelectedSceneObjectActiveChangedSlot();

    /* Show context menu for scene graph */
    void onContextMenuSceneGraph(const QPoint &pos);

    /* Show selected AABB slot */
    void onShowSelectedAABBSlot(int);

    /* Start up scene initialization */
    void onStartUpInitialization();
};

#endif