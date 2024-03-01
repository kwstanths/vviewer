#ifndef __WidgetRightPanel_hpp__
#define __WidgetRightPanel_hpp__

#include <memory>
#include <qboxlayout.h>
#include <qscrollarea.h>
#include <qwidget.h>

#include "vengine/core/Scene.hpp"
#include "vengine/core/SceneObject.hpp"
#include "vengine/core/Engine.hpp"

#include "WidgetName.hpp"
#include "WidgetTransform.hpp"
#include "WidgetComponent.hpp"
#include "WidgetEnvironment.hpp"
#include "WidgetModel3D.hpp"
#include "WidgetMaterial.hpp"
#include "WidgetLight.hpp"

class WidgetRightPanel : public QWidget
{
    Q_OBJECT
public:
    WidgetRightPanel(QWidget *parent, vengine::Engine *engine);

    void setSelectedObject(vengine::SceneObject *object);
    vengine::SceneObject *getSelectedObject() const;

    WidgetEnvironment *getEnvironmentWidget();

    void updateAvailableMaterials();
    void updateAvailableLights();
    void updateAvailableTextures();

    void pauseUpdate(bool pause);

private:
    QVBoxLayout *m_layoutControls;
    QWidget *m_widgetControls;
    QScrollArea *m_widgetScroll;
    WidgetEnvironment *m_widgetEnvironment;

    WidgetName *m_selectedObjectWidgetName = nullptr;
    WidgetTransform *m_selectedObjectWidgetTransform = nullptr;
    WidgetComponent *m_selectedObjectWidgetMaterial = nullptr;
    WidgetComponent *m_selectedObjectWidgetLight = nullptr;
    WidgetComponent *m_selectedObjectWidgetMeshModel = nullptr;

    /* Update timer */
    QTimer *m_updateTimer;

    vengine::Engine *m_engine;
    vengine::SceneObject *m_object;

    void deleteWidgets();
    void createUI(vengine::SceneObject *object);

public Q_SLOTS:
    void onTransformChanged();

private Q_SLOTS:
    void onUpdate();
    void onSceneObjectNameChanged();
    void onSceneObjectActiveChanged(int);
    void onComponentRemoved();
    void onAddComponentMesh();
    void onAddComponentMaterial();
    void onAddComponentPointLight();
    void onAddComponentDirectionalLight();

Q_SIGNALS:
    void selectedSceneObjectNameChanged(QString newName);
    void selectedSceneObjectActiveChanged();
};

#endif