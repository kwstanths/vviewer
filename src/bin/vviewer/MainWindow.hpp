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
    QVulkanInstance * m_vulkanInstance;
    VulkanWindow * m_vulkanWindow;

    QSlider * m_sliderClearColor;

    QWidget * initVulkanWindowWidget();
    QWidget * initControlsWidget();
};

#endif