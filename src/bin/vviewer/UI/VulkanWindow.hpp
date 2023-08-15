#ifndef __VulkanWindow_hpp__
#define __VulkanWindow_hpp__

#include <qwindow.h>
#include <qvulkaninstance.h>
#include <qtimer.h>

#include "vulkan/VulkanEngine.hpp"

#include "vulkan/VulkanScene.hpp"
#include "vulkan/VulkanSwapchain.hpp"
#include "vulkan/renderers/VulkanRenderer.hpp"

class VulkanWindow : public QWindow 
{
    Q_OBJECT
public:
    VulkanWindow();
    ~VulkanWindow();

    VulkanEngine * engine() const;

    std::shared_ptr<Material> importMaterial(std::string name, std::string stackDirectory);
    std::shared_ptr<Material> importZipMaterial(std::string name, std::string filename);

    void windowActicated(bool activated);

protected:

    void releaseResources();

Q_SIGNALS:
    /* emitted when the vulkan initialization has finished */
    void initializationFinished();
    /* emitted when the user selected an object from the 3d scene with the mouse */
    void sceneObjectSelected(std::shared_ptr<SceneObject> object);
    /* emitted when the position of the selected object changes from the gizmo UI controls */
    void selectedObjectPositionChanged();

private:
    bool m_initialized = false;
    VulkanEngine * m_engine = nullptr;

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

private Q_SLOTS:
    void onUpdateCamera();
};

#endif