#ifndef __WidgetEnvironment_hpp__
#define __WidgetEnvironment_hpp__

#include <qwidget.h>
#include <qspinbox.h>
#include <qpushbutton.h>
#include <qslider.h>
#include <qcombobox.h>

#include "core/Materials.hpp"
#include "core/Lights.hpp"
#include "math/Transform.hpp"

#include "WidgetTransform.hpp"

///* A UI widget to represent environment variables */
class WidgetEnvironment : public QWidget {
    Q_OBJECT
public:
    WidgetEnvironment(QWidget* parent, std::shared_ptr<DirectionalLight> light);

    void updateMaps();

private:
    QComboBox* m_comboMaps;
    WidgetTransform* m_lightTransform;
    QPushButton* m_lightColorButton;
    std::shared_ptr<DirectionalLight> m_light;

    void setLightButtonColor();

private slots:
    void onMapChanged(int);
    void onLightColorButton();
    void onLightDirectionChanged(double);
    void onLightColorChanged(QColor color);
};

#endif
