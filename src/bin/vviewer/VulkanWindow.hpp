#ifndef __VulkanWindow_hpp__
#define __VulkanWindow_hpp__

#include <qvulkanwindow.h>
#include <qwindow.h>

#include "VulkanRenderer.hpp"
#include "Camera.hpp"

class VulkanWindow : public QVulkanWindow {
public:
    VulkanWindow();
    QVulkanWindowRenderer * createRenderer() override;

protected:
    void resizeEvent(QResizeEvent *ev) override;

private:

    std::shared_ptr<Camera> m_camera;

};

#endif