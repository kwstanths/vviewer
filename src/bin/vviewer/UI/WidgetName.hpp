#ifndef __WidgetName_hpp__
#define __WidgetName_hpp__

#include <qwidget.h>
#include <qtextedit.h>

/* A UI widget with a text edit */
class WidgetName : public QWidget
{
    Q_OBJECT
public:
    WidgetName(QWidget * parent, QString textName);

    QTextEdit * m_text;
};

#endif