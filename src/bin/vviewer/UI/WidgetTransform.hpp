#ifndef __WidgetTransform_hpp__
#define __WidgetTransform_hpp__

#include <qwidget.h>
#include <qspinbox.h>

#include "math/Transform.hpp"
#include "core/SceneObject.hpp"

/* A UI widget to represent a transform */
class WidgetTransform : public QWidget
{
    Q_OBJECT
public:
    WidgetTransform(QWidget * parent, std::shared_ptr<SceneObject> sceneObject, QString groupBoxName = "Transform:");

    Transform getTransform() const;

    void setTransform(const Transform& transform);

    QDoubleSpinBox *m_positionX, *m_positionY, *m_positionZ;
    QDoubleSpinBox *m_scaleX, *m_scaleY, *m_scaleZ;
    QDoubleSpinBox *m_rotationX, *m_rotationY, *m_rotationZ;
private:

    QWidget * createRow(QString name, QDoubleSpinBox ** X, QDoubleSpinBox ** Y, QDoubleSpinBox ** Z);

    std::shared_ptr<SceneObject> m_sceneObject;

private slots:
    void onTransformChangedSlot(double d);

public slots:
    void onPositionChangedFrom3DScene();

};

#endif