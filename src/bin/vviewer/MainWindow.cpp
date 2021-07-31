#include "MainWindow.hpp"

#include <qvulkaninstance.h>
#include <qlayout.h>
#include <qlabel.h>

#include <utils/Console.hpp>

MainWindow::MainWindow(QWidget *parent) :QMainWindow(parent) {
    
    QWidget * widgetControls = initControlsWidget();
    QWidget * widgetVulkan = initVulkanWindowWidget();

    QHBoxLayout * layout_main = new QHBoxLayout();
    layout_main->addWidget(widgetVulkan);
    layout_main->addWidget(widgetControls);

    QWidget * widget_main = new QWidget();
    widget_main->setLayout(layout_main);

    setCentralWidget(widget_main);
    resize(1600, 900);
}

MainWindow::~MainWindow() {

}

QWidget * MainWindow::initVulkanWindowWidget()
{
    const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
    };

    QByteArrayList layers;
    QByteArrayList extensions;
#ifdef NDEBUG

#else
    //layers.append("VK_LAYER_GOOGLE_threading");
    //layers.append("VK_LAYER_LUNARG_parameter_validation");
    //layers.append("VK_LAYER_LUNARG_object_tracker");
    //layers.append("VK_LAYER_LUNARG_core_validation");
    //layers.append("VK_LAYER_LUNARG_image");
    //layers.append("VK_LAYER_LUNARG_swapchain");
    //layers.append("VK_LAYER_GOOGLE_unique_objects");
    layers.append("VK_LAYER_KHRONOS_validation");

    extensions.append("VK_EXT_debug_utils");
#endif

    m_vulkanInstance = new QVulkanInstance();
    /* Enable validation layers, if available */
    m_vulkanInstance->setLayers(layers);
    m_vulkanInstance->setExtensions(extensions);

    if (!m_vulkanInstance->create()) {
        utils::Console(utils::DebugLevel::FATAL, "Failed to create Vulkan instance: " +
            std::to_string(m_vulkanInstance->errorCode()));
    }

    QVulkanInfoVector<QVulkanLayer> supportedLayers = m_vulkanInstance->supportedLayers();
    for (auto layer : layers) {
        if (!supportedLayers.contains(layer)) {
            utils::Console(utils::DebugLevel::WARNING, "Layer not found: " + std::string(layer));
        }
    }

    QVulkanInfoVector<QVulkanExtension> supportedExtensions = m_vulkanInstance->supportedExtensions();
    for (auto extension : extensions) {
        if (!supportedExtensions.contains(extension)) {
            utils::Console(utils::DebugLevel::WARNING, "Extension not found: " + std::string(extension));
        }
    }

    m_vulkanWindow = new VulkanWindow();
    m_vulkanWindow->setVulkanInstance(m_vulkanInstance);

    return QWidget::createWindowContainer(m_vulkanWindow);
}

QWidget * MainWindow::initControlsWidget()
{
    QVBoxLayout * layout_controls = new QVBoxLayout();
    layout_controls->addWidget(new QLabel("test"));
    QWidget * widget_controls = new QWidget();
    widget_controls->setLayout(layout_controls);

    widget_controls->setFixedWidth(100);
    return widget_controls;
}
