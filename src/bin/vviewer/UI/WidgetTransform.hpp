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
    WidgetTransform(QWidget * parent, SceneObject * object);

    Transform getTransform() const;

    void setTransform(const Transform& transform);

    QDoubleSpinBox *m_positionX, *m_positionY, *m_positionZ;
    QDoubleSpinBox *m_scaleX, *m_scaleY, *m_scaleZ;
    QDoubleSpinBox *m_rotationX, *m_rotationY, *m_rotationZ;
private:

    QWidget * createRow(QString name, QDoubleSpinBox ** X, QDoubleSpinBox ** Y, QDoubleSpinBox ** Z);

    SceneObject * m_object;

private slots:
    void onTransformChangedSlot(double d);

};

#endif