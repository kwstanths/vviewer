#ifndef __VulkanWindow_hpp__
#define __VulkanWindow_hpp__

#include <qvulkanwindow.h>
#include <qwindow.h>
#include <qtimer.h>

#include <math/Transform.hpp>
#include <core/Camera.hpp>
#include <core/AssetManager.hpp>
#include <core/MeshModel.hpp>
#include <core/Lights.hpp>
#include <core/Scene.hpp>

#include "renderers/VulkanRenderer.hpp"
#include "VulkanScene.hpp"

class VulkanWindow : public QVulkanWindow {
    friend VulkanRenderer;
    Q_OBJECT
public:
    VulkanWindow();
    ~VulkanWindow();
    QVulkanWindowRenderer * createRenderer() override;

    Material* importMaterial(std::string name, std::string stackDirectory);
    Material* importZipMaterial(std::string name, std::string filename);

    VulkanRenderer * m_renderer = nullptr;

    VulkanScene* m_scene = nullptr;

    float delta() const;

signals:
    /* emitted when the vulkan initialization has finished */
    void initializationFinished();
    /* emitted when the selected object changes from the 3D scene */
    void sceneObjectSelected(std::shared_ptr<SceneObject> object);
    /* emitted when the position of the selected object changes from the UI controls */
    void selectedObjectPositionChanged();

protected:
    void resizeEvent(QResizeEvent *ev) override;
    void keyPressEvent(QKeyEvent *ev) override;
    void keyReleaseEvent(QKeyEvent *ev) override;
    void mousePressEvent(QMouseEvent *ev) override;
    void mouseReleaseEvent(QMouseEvent *ev) override;
    void mouseMoveEvent(QMouseEvent *ev) override;

private:
    std::unordered_map<int, bool> m_keysPressed;
    bool m_mousePosFirst = true;
    QPointF m_mousePos;

    ID m_selectedPressed = 0;

    QTimer * m_updateCameraTimer;

    void vulkanInitializationFinished();

private slots:
    void onUpdateCamera();

};

#endif