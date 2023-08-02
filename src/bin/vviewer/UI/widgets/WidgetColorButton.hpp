#ifndef __WidgetColorButton_hpp__
#define __WidgetColorButton_hpp__

#include <qwidget.h>
#include <qpushbutton.h>

#include <glm/glm.hpp>

class WidgetColorButton : public QWidget {
    Q_OBJECT
public:
    WidgetColorButton(QWidget * parent, glm::vec3 color);

    void setColor(glm::vec3 color);

private:
    QPushButton* m_colorButton;
    QColor m_color;

private slots:
    void onColorButtonPressed();
    void onColorChanged(QColor color);

signals:
    void colorChanged(glm::vec3);
};

#endif