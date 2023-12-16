#include "WidgetComponent.hpp"
#include <qboxlayout.h>
#include <qlabel.h>
#include <qnamespace.h>
#include <qpushbutton.h>

using namespace vengine;

WidgetComponent::WidgetComponent(QWidget *parent, UIComponentWrapper *componentWrapper, Engine *engine)
    : QWidget(parent)
    , m_componentWrapper(componentWrapper)
    , m_engine(engine)
{
    m_widgetEncapsulated = m_componentWrapper->generateWidget();

    QLabel *label = new QLabel(m_componentWrapper->m_name);
    label->setStyleSheet("font-weight: bold");

    QHBoxLayout *layoutLeftHeadline = new QHBoxLayout();
    layoutLeftHeadline->addWidget(label);
    layoutLeftHeadline->setContentsMargins(0, 0, 0, 0);
    layoutLeftHeadline->setAlignment(Qt::AlignLeft);
    QWidget *widgetLeftHeadline = new QWidget();
    widgetLeftHeadline->setLayout(layoutLeftHeadline);

    QPushButton *buttonRemove = new QPushButton(tr("Remove"));
    connect(buttonRemove, &QPushButton::released, this, &WidgetComponent::onRemoved);

    QHBoxLayout *layoutRightHeadline = new QHBoxLayout();
    layoutRightHeadline->addWidget(buttonRemove);
    layoutRightHeadline->setContentsMargins(0, 0, 0, 0);
    layoutRightHeadline->setAlignment(Qt::AlignRight);
    QWidget *widgetRightHeadline = new QWidget();
    widgetRightHeadline->setLayout(layoutRightHeadline);

    QHBoxLayout *layoutHeadline = new QHBoxLayout();
    layoutHeadline->addWidget(widgetLeftHeadline);
    layoutHeadline->addWidget(widgetRightHeadline);
    layoutHeadline->setAlignment(Qt::AlignBottom);
    layoutHeadline->setContentsMargins(0, 0, 0, 0);
    QWidget *widgetHeadline = new QWidget();
    widgetHeadline->setLayout(layoutHeadline);

    QVBoxLayout *layoutMain = new QVBoxLayout();
    layoutMain->addWidget(widgetHeadline);
    layoutMain->addWidget(m_widgetEncapsulated);
    layoutMain->setContentsMargins(0, 0, 0, 0);
    layoutMain->setAlignment(Qt::AlignTop);
    layoutMain->setSpacing(0);
    setLayout(layoutMain);

    updateHeight();
}

WidgetComponent::~WidgetComponent()
{
    delete m_componentWrapper;
    delete m_widgetEncapsulated;
}

void WidgetComponent::updateHeight()
{
    setFixedHeight(m_componentWrapper->getWidgetHeight() + 10);
}

void WidgetComponent::onRemoved()
{
    m_engine->stop();
    m_engine->waitIdle();
    m_componentWrapper->removeComponent();
    m_engine->start();

    Q_EMIT componentRemoved();
}
