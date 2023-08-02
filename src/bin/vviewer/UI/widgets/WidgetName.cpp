#include "WidgetName.hpp"

#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>

WidgetName::WidgetName(QWidget * parent, QString textName) : QWidget(parent)
{
    m_text = new QTextEdit(textName);
    m_text->setPlainText(textName);

    QFont font;
    font.setBold(true);

    QGroupBox * groupBox = new QGroupBox(tr("Name"));
    groupBox->setStyleSheet("QGroupBox { font-weight: bold; } ");
    QVBoxLayout * layoutTest = new QVBoxLayout();
    layoutTest->addWidget(m_text);
    layoutTest->setContentsMargins(5, 5, 5, 5);

    groupBox->setLayout(layoutTest);

    QVBoxLayout * layoutMain = new QVBoxLayout();
    layoutMain->addWidget(groupBox);
    layoutMain->setContentsMargins(0, 0, 0, 0);
    layoutMain->setAlignment(Qt::AlignTop);
    setLayout(layoutMain);
    setFixedHeight(60);
}
