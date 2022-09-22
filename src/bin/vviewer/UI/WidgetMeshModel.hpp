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
    WidgetMeshModel(QWidget * parent, SceneObject * sceneObject);

    std::string getSelectedModel() const;
    std::string getSelectedMesh() const;

private:
    SceneObject * m_sceneObject = nullptr;
    QComboBox * m_models = nullptr;
    QComboBox * m_meshes = nullptr;
    const MeshModel * m_meshModel;

    QStringList getModelMeshes(const MeshModel * model);

private slots:
    void onMeshModelChangedSlot(int);
    void onMeshChangedSlot(int);

};

#endif