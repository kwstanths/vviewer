#include "WidgetTransform.hpp"

#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>

#include <glm/glm.hpp>

WidgetTransform::WidgetTransform(QWidget * parent, std::shared_ptr<SceneNode> sceneNode, QString groupBoxName) : QWidget(parent)
{
    m_sceneNode = sceneNode;

    QGroupBox * groupBox = new QGroupBox(groupBoxName);
    QVBoxLayout * layoutTest = new QVBoxLayout();
    layoutTest->addWidget(createRow("P:", &m_positionX, &m_positionY, &m_positionZ));
    layoutTest->addWidget(createRow("S:", &m_scaleX, &m_scaleY, &m_scaleZ));
    layoutTest->addWidget(createRow("R:", &m_rotationX, &m_rotationY, &m_rotationZ));
    layoutTest->setContentsMargins(5, 5, 5, 5);

    m_scaleX->setValue(1.0f);
    m_scaleY->setValue(1.0f);
    m_scaleZ->setValue(1.0f);

    groupBox->setLayout(layoutTest);

    QVBoxLayout * layoutMain = new QVBoxLayout();
    layoutMain->addWidget(groupBox);
    layoutMain->setContentsMargins(0, 0, 0, 0);
    setLayout(layoutMain);
    setFixedHeight(130);

    if (m_sceneNode != nullptr) setTransform(m_sceneNode->m_localTransform);

    connect(m_positionX, SIGNAL(valueChanged(double)), this, SLOT(onTransformChangedSlot(double)));
    connect(m_positionY, SIGNAL(valueChanged(double)), this, SLOT(onTransformChangedSlot(double)));
    connect(m_positionZ, SIGNAL(valueChanged(double)), this, SLOT(onTransformChangedSlot(double)));
    connect(m_scaleX, SIGNAL(valueChanged(double)), this, SLOT(onTransformChangedSlot(double)));
    connect(m_scaleY, SIGNAL(valueChanged(double)), this, SLOT(onTransformChangedSlot(double)));
    connect(m_scaleZ, SIGNAL(valueChanged(double)), this, SLOT(onTransformChangedSlot(double)));
    connect(m_rotationX, SIGNAL(valueChanged(double)), this, SLOT(onTransformChangedSlot(double)));
    connect(m_rotationY, SIGNAL(valueChanged(double)), this, SLOT(onTransformChangedSlot(double)));
    connect(m_rotationZ, SIGNAL(valueChanged(double)), this, SLOT(onTransformChangedSlot(double)));
}

Transform WidgetTransform::getTransform() const
{
    Transform transform;
    transform.setPosition(m_positionX->value(), m_positionY->value(), m_positionZ->value());
    transform.setScale(m_scaleX->value(), m_scaleY->value(), m_scaleZ->value());
    transform.setRotationEuler(glm::radians(m_rotationX->value()), glm::radians(m_rotationY->value()), glm::radians(m_rotationZ->value()));
    return transform;
}

void WidgetTransform::setTransform(const Transform& transform)
{
    glm::vec3 position = transform.getPosition();
    m_positionX->setValue(position.x);
    m_positionY->setValue(position.y);
    m_positionZ->setValue(position.z);

    glm::vec3 scale = transform.getScale();
    m_scaleX->setValue(scale.x);
    m_scaleY->setValue(scale.y);
    m_scaleZ->setValue(scale.z);

    glm::vec3 rotation = glm::eulerAngles(transform.getRotation());
    m_rotationX->setValue(glm::degrees(rotation.x));
    m_rotationY->setValue(glm::degrees(rotation.y));
    m_rotationZ->setValue(glm::degrees(rotation.z));
}

QWidget * WidgetTransform::createRow(QString name, QDoubleSpinBox ** X, QDoubleSpinBox ** Y, QDoubleSpinBox ** Z)
{
    QLabel * labelTitle = new QLabel(name);
    labelTitle->setFixedWidth(20);

    *X = new QDoubleSpinBox();
    *Y = new QDoubleSpinBox();
    *Z = new QDoubleSpinBox();

    (*X)->setMaximum(1000);
    (*Y)->setMaximum(1000);
    (*Z)->setMaximum(1000);
    (*X)->setMinimum(-1000);
    (*Y)->setMinimum(-1000);
    (*Z)->setMinimum(-1000);

    QHBoxLayout * layoutX = new QHBoxLayout();
    layoutX->addWidget(new QLabel("X:"));
    layoutX->addWidget(*X);
    layoutX->setContentsMargins(0, 0, 0, 0);
    layoutX->setAlignment(Qt::AlignLeft);
    QWidget * widgetX = new QWidget();
    widgetX->setLayout(layoutX);
    QHBoxLayout * layoutY = new QHBoxLayout();
    layoutY->addWidget(new QLabel("Y:"));
    layoutY->addWidget(*Y);
    layoutY->setContentsMargins(0, 0, 0, 0);
    layoutY->setAlignment(Qt::AlignLeft);
    QWidget * widgetY = new QWidget();
    widgetY->setLayout(layoutY);
    QHBoxLayout * layoutZ = new QHBoxLayout();
    layoutZ->addWidget(new QLabel("Z:"));
    layoutZ->addWidget(*Z);
    layoutZ->setContentsMargins(0, 0, 0, 0);
    layoutZ->setAlignment(Qt::AlignLeft);
    QWidget * widgetZ = new QWidget();
    widgetZ->setLayout(layoutZ);

    QHBoxLayout * layoutControls = new QHBoxLayout();
    layoutControls->addWidget(widgetX);
    layoutControls->addWidget(widgetY);
    layoutControls->addWidget(widgetZ);
    layoutControls->setContentsMargins(0, 0, 0, 0);
    QWidget * widgetControls = new QWidget();
    widgetControls->setLayout(layoutControls);

    QHBoxLayout * layoutMain = new QHBoxLayout();
    layoutMain->addWidget(labelTitle);
    layoutMain->addWidget(widgetControls);
    layoutMain->setContentsMargins(0, 0, 0, 0);
    QWidget * widgetMain = new QWidget();
    widgetMain->setLayout(layoutMain);

    return widgetMain;
}

void WidgetTransform::onTransformChangedSlot(double d) {
    if (m_sceneNode != nullptr) m_sceneNode->m_localTransform = getTransform();
}
