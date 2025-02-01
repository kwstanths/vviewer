#ifndef __WidgetComponentLight_hpp__
#define __WidgetComponentLight_hpp__

#include <qcombobox.h>
#include <qwidget.h>
#include <qpushbutton.h>
#include <qslider.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qcheckbox.h>

#include <vengine/core/Light.hpp>
#include <vengine/utils/ECS.hpp>

/* A UI widget for a light component */
class WidgetComponentLight : public QWidget
{
    Q_OBJECT
public:
    static const int HEIGHT = 175;
    WidgetComponentLight(QWidget *parent, vengine::ComponentLight &lightComponent);

    void updateAvailableLights();

private:
    vengine::ComponentLight &m_lightComponent;

    QComboBox *m_comboBoxLights = nullptr;
    QWidget *m_widgetLight = nullptr;
    QCheckBox *m_checkBoxCastShadows;
    QLabel *m_labelLight;
    QLabel *m_labelCastShadows;

    QWidget *m_widgetGroupBox = nullptr;
    QVBoxLayout *m_layoutGroupBox = nullptr;
    QVBoxLayout *m_layoutMain = nullptr;

    void createUI(QWidget *widgetLight);
    QWidget *createLightWidget(vengine::Light *light);

private Q_SLOTS:
    void onLightChanged(int);
    void onCheckBoxCastShadows(int);
};

#endif