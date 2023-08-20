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
    static const int HEIGHT = 90;

    WidgetMeshModel(QWidget * parent, vengine::ComponentMesh& meshComponent);

    std::string getSelectedModel() const;
    std::string getSelectedMesh() const;

    uint32_t getHeight() const;

private:
    vengine::ComponentMesh& m_meshComponent;
    const vengine::MeshModel * m_meshModel;
    QComboBox * m_models = nullptr;
    QComboBox * m_meshes = nullptr;

    QStringList getModelMeshes(const vengine::MeshModel* model);

private Q_SLOTS:
    void onMeshModelChangedSlot(int);
    void onMeshChangedSlot(int);

};

#endif