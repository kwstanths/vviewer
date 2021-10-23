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
    /* UI menu things */
    QMenu * m_menuFile;
    QAction * m_actionImport;
    QAction * m_actionAddSceneObject;

    /* A UI list widget with all the scene objects*/
    QListWidget * m_sceneObjects;
    int m_nObjects = 0;
    WidgetName * m_selectedObjectWidgetName = nullptr;
    WidgetTransform * m_selectedObjectWidgetTransform = nullptr;
    WidgetMeshModel * m_selectedObjectWidgetMeshModel = nullptr;
    QWidget * m_selectedObjectWidgetMaterial = nullptr;

    QWidget * initLeftPanel();
    QWidget * initVulkanWindowWidget();
    QWidget * initControlsWidget();
    void createMenu();
    QStringList getImportedModels();

private slots:
    void onImportModelSlot();
    void onAddSceneObjectSlot();
    void onSelectedSceneObjectChangedSlot();
    
    void onSelectedSceneObjectNameChangedSlot();
    void onSelectedSceneObjectMeshModelChangedSlot(int);
};

#endif