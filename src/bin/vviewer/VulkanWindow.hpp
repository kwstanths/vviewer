#ifndef __VulkanWindow_hpp__
#define __VulkanWindow_hpp__

#include <qvulkanwindow.h>
#include <qwindow.h>
#include <qtimer.h>

#include <math/Transform.hpp>
#include <core/Camera.hpp>
#include <core/AssetManager.hpp>
#include <core/MeshModel.hpp>

#include "VulkanRenderer.hpp"

class VulkanWindow : public QVulkanWindow {
    Q_OBJECT
public:
    VulkanWindow();
    ~VulkanWindow();
    QVulkanWindowRenderer * createRenderer() override;

    bool ImportMeshModel(std::string filename);

    SceneObject * AddSceneObject(std::string filename, Transform transform);

protected:
    void resizeEvent(QResizeEvent *ev) override;
    void keyPressEvent(QKeyEvent *ev) override;
    void keyReleaseEvent(QKeyEvent *ev) override;
    void mousePressEvent(QMouseEvent *ev) override;
    void mouseReleaseEvent(QMouseEvent *ev) override;
    void mouseMoveEvent(QMouseEvent *ev) override;

private:
    VulkanRenderer * m_renderer = nullptr;

    std::unordered_map<int, bool> m_keysPressed;

    bool m_mousePosFirst = true;
    QPointF m_mousePos;

    std::shared_ptr<Camera> m_camera;
    QTimer * m_updateCameraTimer;

private slots:
    void onUpdateCamera();

};

#endif