#ifndef __WidgetMeshModel_hpp__
#define __WidgetMeshModel_hpp__

#include <qwidget.h>
#include <qcombobox.h>

#include "core/SceneObject.hpp"

/* A UI widget to represent a mesh model */
class WidgetMeshModel : public QWidget
{
    Q_OBJECT
public:
    WidgetMeshModel(QWidget * parent, SceneObject * sceneObject, QStringList availableModels);

    std::string getSelectedModel() const;

    QComboBox * m_models = nullptr;
private:
    SceneObject * m_sceneObject = nullptr;

private slots:
    void onMeshModelChangedSlot(int);

};

#endif