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

#include "VulkanRenderer.hpp"
#include "VulkanScene.hpp"

class VulkanWindow : public QVulkanWindow {
    Q_OBJECT
public:
    VulkanWindow();
    ~VulkanWindow();
    QVulkanWindowRenderer * createRenderer() override;

    VulkanRenderer * m_renderer = nullptr;

    VulkanScene* m_scene = nullptr;

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

    QTimer * m_updateCameraTimer;

private slots:
    void onUpdateCamera();

};

#endif