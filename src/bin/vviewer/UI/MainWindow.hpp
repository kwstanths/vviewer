#ifndef __MainWindow_hpp__
#define __MainWindow_hpp__

#include <qmainwindow.h>
#include <qwidget.h>
#include <qslider.h>
#include <qlistwidget.h>
#include <qlayout.h>

#include "WidgetName.hpp"
#include "WidgetTransform.hpp"

#include "VulkanWindow.hpp"

Q_DECLARE_METATYPE(SceneObject*)

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent);
    virtual ~MainWindow();

private:
    QWidget * initLeftPanel();
    QWidget * initVulkanWindowWidget();
    
    QWidget * initControlsWidget();
    QVBoxLayout * m_layoutControls;

    QVulkanInstance * m_vulkanInstance;
    VulkanWindow * m_vulkanWindow;

    void createMenu();
    QMenu * m_menuFile;
    QAction * m_actionImport;
    QAction * m_actionAddSceneObject;

    QStringList m_importedModels;

    QListWidget * m_sceneObjects;
    int m_nObjects = 0;
    WidgetName * m_selectedObjectWidgetName = nullptr;
    WidgetTransform * m_selectedObjectWidgetTransform = nullptr;

private slots:
    void onImportModelSlot();
    void onAddSceneObjectSlot();
    void onSelectedSceneObjectChangedSlot();
    void onSelectedSceneObjectNameChangedSlot();
    void onSelectedSceneObjectTransformChangedSlot(double d);
};

#endif