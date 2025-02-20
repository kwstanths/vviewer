#ifndef __WidgetComponentModel3D_hpp__
#define __WidgetComponentModel3D_hpp__

#include <qwidget.h>
#include <qcombobox.h>

#include "vengine/core/SceneObject.hpp"

/* A UI widget to represent a model3D component */
class WidgetComponentModel3D : public QWidget
{
    Q_OBJECT
public:
    static const int HEIGHT = 90;

    WidgetComponentModel3D(QWidget *parent, vengine::ComponentMesh &meshComponent);

    std::string getSelectedModel() const;
    std::string getSelectedMesh() const;

    uint32_t getHeight() const;

private:
    vengine::ComponentMesh &m_meshComponent;
    const vengine::Model3D *m_model;
    QComboBox *m_models = nullptr;
    QComboBox *m_meshes = nullptr;

    QStringList getModelMeshes(const vengine::Model3D *model);

private Q_SLOTS:
    void onMeshModelChangedSlot(int);
    void onMeshChangedSlot(int);
};

#endif