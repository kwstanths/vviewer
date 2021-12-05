#ifndef __MainWindow_hpp__
#define __MainWindow_hpp__

#include <qmainwindow.h>
#include <qwidget.h>
#include <qslider.h>
#include <qlistwidget.h>
#include <qlayout.h>

#include "WidgetName.hpp"
#include "WidgetTransform.hpp"
#include "WidgetMeshModel.hpp"
#include "WidgetMaterialPBR.hpp"
#include "WidgetEnvironment.hpp"

#include "VulkanWindow.hpp"

Q_DECLARE_METATYPE(SceneObject*)

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

    /* A UI list widget with all the scene objects*/
    QListWidget * m_sceneObjects;
    int m_nObjects = 0;
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
    void onImportModelSlot();
    void onImportTextureColorSlot();
    void onImportTextureOtherSlot();
    void onImportTextureHDRSlot();
    void onImportEnvironmentMap();
    void onAddSceneObjectSlot();
    void onCreateMaterialSlot();
    
    void onSelectedSceneObjectChangedSlot();
    void onSelectedSceneObjectNameChangedSlot();
};

#endif