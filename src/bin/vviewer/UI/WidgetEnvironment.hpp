#ifndef __WidgetEnvironment_hpp__
#define __WidgetEnvironment_hpp__

#include <qwidget.h>
#include <qspinbox.h>
#include <qpushbutton.h>
#include <qslider.h>
#include <qcombobox.h>

#include "core/Materials.hpp"

///* A UI widget to represent environment variables */
class WidgetEnvironment : public QWidget {
    Q_OBJECT
public:
    WidgetEnvironment(QWidget* parent);

    void updateMaps();

private:
    QComboBox* m_comboMaps;

private slots:
    void onMapChanged(int);

};

#endif
