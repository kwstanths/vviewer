#include "WidgetName.hpp"

#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>

WidgetName::WidgetName(QWidget * parent, QString textName) : QWidget(parent)
{
    m_text = new QTextEdit(textName);
    m_text->setPlainText(textName);

    QGroupBox * groupBox = new QGroupBox(tr("Name"));
    QVBoxLayout * layoutTest = new QVBoxLayout();
    layoutTest->addWidget(m_text);
    layoutTest->setContentsMargins(0, 0, 0, 0);

    groupBox->setLayout(layoutTest);

    QVBoxLayout * layoutMain = new QVBoxLayout();
    layoutMain->addWidget(groupBox);
    layoutMain->setContentsMargins(0, 0, 0, 0);
    layoutMain->setAlignment(Qt::AlignTop);
    setLayout(layoutMain);
    setFixedHeight(45);
}
