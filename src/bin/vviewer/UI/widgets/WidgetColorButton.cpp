#include "WidgetColorButton.hpp"

#include <qlayout.h>
#include <qlabel.h>
#include <qcolordialog.h>

#include "UI/UIUtils.hpp"

WidgetColorButton::WidgetColorButton(QWidget * parent, glm::vec3 color) : QWidget(parent)
{
    m_color = QColor(color.r * 255, color.g * 255, color.b * 255);

    m_colorButton = new QPushButton();
    m_colorButton->setFixedWidth(25);
    setButtonColor(m_colorButton, m_color);
    connect(m_colorButton, SIGNAL(pressed()), this, SLOT(onColorButtonPressed()));


    QHBoxLayout* layout = new QHBoxLayout();
    layout->addWidget(new QLabel("Color: "));
    layout->addWidget(m_colorButton);
    layout->setContentsMargins(0, 0, 0, 0);
    
    setLayout(layout);
}

void WidgetColorButton::setColor(glm::vec3 color)
{
    m_color = QColor(color.r * 255, color.g * 255, color.b * 255);
    setButtonColor(m_colorButton, m_color);
}

void WidgetColorButton::onColorButtonPressed()
{
    QColorDialog* dialog = new QColorDialog(nullptr);
    dialog->adjustSize();
    connect(dialog, SIGNAL(currentColorChanged(QColor)), this, SLOT(onColorChanged(QColor)));
    dialog->exec();

    setButtonColor(m_colorButton, m_color);
}

void WidgetColorButton::onColorChanged(QColor color)
{
    m_color = color;

    Q_EMIT colorChanged(glm::vec3(color.red(), color.green(), color.blue()) / glm::vec3(255.0f));
}

