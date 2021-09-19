#ifndef __MainWindow_hpp__
#define __MainWindow_hpp__

#include <qmainwindow.h>
#include <qwidget.h>
#include <qslider.h>

#include "VulkanWindow.hpp"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent);
    virtual ~MainWindow();

private:
    QWidget * initVulkanWindowWidget();
    QWidget * initControlsWidget();
    QVulkanInstance * m_vulkanInstance;
    VulkanWindow * m_vulkanWindow;

    void createMenu();
    QMenu * m_menuFile;
    QAction * m_actionImport;
    QAction * m_actionAddSceneObject;

    QStringList m_importedModels;

private slots:
    void onImportModelSlot();
    void onAddSceneObjectSlot();
};

#endif