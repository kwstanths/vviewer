#ifndef __VulkanWindow_hpp__
#define __VulkanWindow_hpp__

#include <qwindow.h>
#include <qvulkaninstance.h>
#include <qtimer.h>

#include "VulkanCore.hpp"
#include "VulkanScene.hpp"
#include "VulkanSwapchain.hpp"
#include "renderers/VulkanRenderer.hpp"

class VulkanWindow : public QWindow 
{
    Q_OBJECT
public:
    VulkanWindow();
    ~VulkanWindow();

    VulkanScene* getScene() const;
    VulkanRenderer * getRenderer() const;

    float delta() const;

    std::shared_ptr<Material> importMaterial(std::string name, std::string stackDirectory);
    std::shared_ptr<Material> importZipMaterial(std::string name, std::string filename);

    void windowActicated(bool activated);

protected:

    VulkanScene * m_scene;

    void releaseResources();

signals:
    /* emitted when the vulkan initialization has finished */
    void initializationFinished();
    /* emitted when the user selected an object from the 3d scene with the mouse */
    void sceneObjectSelected(std::shared_ptr<SceneObject> object);
    /* emitted when the position of the selected object changes from the gizmo UI controls */
    void selectedObjectPositionChanged();

private:
    bool m_initialized = false;
    VulkanCore m_vkcore;

    VulkanSwapchain * m_swapchain = nullptr;
    VulkanRenderer * m_renderer = nullptr;
    bool m_renderPaused = true;

    std::unordered_map<int, bool> m_keysPressed;
    bool m_mousePosFirst = true;
    QPointF m_mousePos;
    QTimer * m_updateCameraTimer;

    ID m_selectedPressed = 0;

    void exposeEvent(QExposeEvent* event) override;
    bool event(QEvent* event) override;
    void resizeEvent(QResizeEvent *ev) override;
    void keyPressEvent(QKeyEvent *ev) override;
    void keyReleaseEvent(QKeyEvent *ev) override;
    void mousePressEvent(QMouseEvent *ev) override;
    void mouseReleaseEvent(QMouseEvent *ev) override;
    void mouseMoveEvent(QMouseEvent *ev) override;

private slots:
    void onUpdateCamera();
};

#endif