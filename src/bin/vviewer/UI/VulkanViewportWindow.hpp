#ifndef __VulkanViewportWindow_hpp__
#define __VulkanViewportWindow_hpp__

#include <qwindow.h>
#include <qvulkaninstance.h>
#include <qtimer.h>

#include "ViewportWindow.hpp"

#include "vengine/vulkan/VulkanEngine.hpp"
#include "vengine/vulkan/VulkanScene.hpp"
#include "vengine/vulkan/VulkanSwapchain.hpp"
#include "vengine/vulkan/renderers/VulkanRenderer.hpp"

class VulkanViewportWindow : public ViewportWindow
{
    Q_OBJECT
public:
    VulkanViewportWindow();
    ~VulkanViewportWindow();

    vengine::Engine *engine() const override;

    void windowActicated(bool activated) override;

protected:
    void releaseResources();

private:
    bool m_initialized = false;
    QVulkanInstance *m_vkinstance = nullptr;
    vengine::VulkanEngine *m_engine = nullptr;

    std::unordered_map<int, bool> m_keysPressed;
    bool m_mousePosFirst = true;
    QPointF m_mousePos;
    QTimer *m_updateCameraTimer;

    vengine::ID m_selectedPressed = 0;

    void exposeEvent(QExposeEvent *event) override;
    bool event(QEvent *event) override;
    void resizeEvent(QResizeEvent *ev) override;
    void keyPressEvent(QKeyEvent *ev) override;
    void keyReleaseEvent(QKeyEvent *ev) override;
    void mousePressEvent(QMouseEvent *ev) override;
    void mouseReleaseEvent(QMouseEvent *ev) override;
    void mouseMoveEvent(QMouseEvent *ev) override;
    void wheelEvent(QWheelEvent *ev) override;

private Q_SLOTS:
    void onUpdateCamera();
};

#endif