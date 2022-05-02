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
#include "WidgetMaterialPBR.hpp"
#include "WidgetEnvironment.hpp"

#include "core/Scene.hpp"
#include "vulkan/VulkanWindow.hpp"

Q_DECLARE_METATYPE(std::shared_ptr<SceneNode>)

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

    /* Widgets that appear on the controls on the right panel */
    WidgetName * m_selectedObjectWidgetName = nullptr;
    WidgetTransform * m_selectedObjectWidgetTransform = nullptr;
    WidgetMeshModel * m_selectedObjectWidgetMeshModel = nullptr;
    QWidget * m_selectedObjectWidgetMaterial = nullptr;
    WidgetEnvironment * m_widgetEnvironment = nullptr;

    QWidget * initLeftPanel();
    QWidget * initVulkanWindowWidget();
    QWidget * initControlsWidget();
    void createMenu();

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
    /* Add an object in the scene at root */
    void onAddSceneObjectRootSlot();
    /* Add an object in the scene as a child to the currently selected node */
    void onAddSceneObjectSlot();
    /* Remove the selected object from the scene */
    void onRemoveSceneObjectSlot();
    /* Create a material */
    void onCreateMaterialSlot();
    /* Render scene */
    void onRenderSceneSlot();
    
    /* Currently selected item in the scene changed */
    void onSelectedSceneObjectChangedSlot();
    /* Currently selected item's name in the scene changed */
    void onSelectedSceneObjectNameChangedSlot();

    /* Show context menu for scene graph */
    void onContextMenuSceneGraph(const QPoint& pos);
};

#endif