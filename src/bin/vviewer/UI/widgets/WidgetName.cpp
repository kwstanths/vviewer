#include "WidgetName.hpp"

#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>

WidgetName::WidgetName(QWidget *parent, QString textName)
    : QWidget(parent)
{
    m_text = new QTextEdit(textName);
    m_text->setPlainText(textName);

    QVBoxLayout *layoutTest = new QVBoxLayout();
    layoutTest->addWidget(m_text);
    layoutTest->setContentsMargins(0, 0, 0, 0);
    layoutTest->setAlignment(Qt::AlignTop);
    setLayout(layoutTest);
    setFixedHeight(25);
}
