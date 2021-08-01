#include "VulkanWindow.hpp"
#include <qevent.h>

VulkanWindow::VulkanWindow()
{
    //OrthographicCamera * camera = new OrthographicCamera();
    //camera->setOrthoWidth(10);
    //m_camera = std::shared_ptr<Camera>(camera);

    PerspectiveCamera * camera = new PerspectiveCamera();
    m_camera = std::shared_ptr<Camera>(camera);
}

QVulkanWindowRenderer * VulkanWindow::createRenderer()
{
    VulkanRenderer * renderer = new VulkanRenderer(this);

    renderer->setCamera(m_camera);

    return renderer;
}

void VulkanWindow::resizeEvent(QResizeEvent * ev)
{
    m_camera->setWindowSize(ev->size().width(), ev->size().height());
}
