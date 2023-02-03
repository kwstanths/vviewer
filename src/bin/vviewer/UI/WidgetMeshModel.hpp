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
    WidgetMeshModel(QWidget * parent, std::shared_ptr<SceneObject> sceneObject);

    std::string getSelectedModel() const;
    std::string getSelectedMesh() const;

private:
    std::shared_ptr<SceneObject> m_sceneObject = nullptr;
    const MeshModel * m_meshModel;
    QComboBox * m_models = nullptr;
    QComboBox * m_meshes = nullptr;

    QStringList getModelMeshes(const MeshModel* model);

private slots:
    void onMeshModelChangedSlot(int);
    void onMeshChangedSlot(int);

};

#endif