#include "WidgetName.hpp"

#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>

WidgetName::WidgetName(QWidget * parent, QString textName) : QWidget(parent)
{
    m_text = new QTextEdit(textName);
    m_text->setPlainText(textName);

    QGroupBox * groupBox = new QGroupBox(tr("Name"));
    QHBoxLayout * layoutTest = new QHBoxLayout();
    layoutTest->addWidget(m_text);
    layoutTest->setContentsMargins(0, 0, 0, 0);

    groupBox->setLayout(layoutTest);

    QHBoxLayout * layoutMain = new QHBoxLayout();
    layoutMain->addWidget(groupBox);
    layoutMain->setContentsMargins(0, 0, 0, 0);
    setLayout(layoutMain);
    setFixedHeight(45);
}
