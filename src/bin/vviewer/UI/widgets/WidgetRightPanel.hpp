#ifndef __WidgetRightPanel_hpp__
#define __WidgetRightPanel_hpp__

#include <memory>
#include <qboxlayout.h>
#include <qscrollarea.h>
#include <qwidget.h>

#include "core/Scene.hpp"
#include "core/SceneObject.hpp"

#include "WidgetName.hpp"
#include "WidgetTransform.hpp"
#include "WidgetComponent.hpp"
#include "WidgetEnvironment.hpp"
#include "WidgetMeshModel.hpp"
#include "WidgetMaterial.hpp"
#include "WidgetLight.hpp"

class WidgetRightPanel : public QWidget
{
    Q_OBJECT
public:
    WidgetRightPanel(QWidget * parent, Scene * scene);

    void setSelectedObject(std::shared_ptr<SceneObject> object);

    WidgetEnvironment * getEnvironmentWidget();

    void updateAvailableMaterials();

private:
    QVBoxLayout * m_layoutControls;
    QWidget * m_widgetControls;
    QScrollArea * m_widgetScroll;
    WidgetEnvironment * m_widgetEnvironment;

    WidgetName * m_selectedObjectWidgetName = nullptr;
    WidgetTransform * m_selectedObjectWidgetTransform = nullptr;
    WidgetComponent * m_selectedObjectWidgetMaterial = nullptr;
    WidgetComponent * m_selectedObjectWidgetLight = nullptr;
    WidgetComponent * m_selectedObjectWidgetMeshModel = nullptr;

    /* Update timer */
    QTimer* m_updateTimer;

    std::shared_ptr<SceneObject> m_object;

    void deleteWidgets();
    void createUI(std::shared_ptr<SceneObject> object);

public slots:
    void onTransformChanged();

private slots:
    void onUpdate();
    void onSceneObjectNameChanged();
    void onComponentRemoved();
    void onAddComponentMesh();
    void onAddComponentMaterial();
    void onAddComponentLight();

signals:
    void selectedSceneObjectNameChanged(QString newName);
};

#endif