#include "DialogAddSceneObject.hpp"

#include <qlayout.h>
#include <qgroupbox.h>

DialogAddSceneObject::DialogAddSceneObject(QWidget *parent, const char *name, QStringList availableModels) : QDialog(parent)
{
    setWindowTitle(QString(name));

    m_widgetMeshModel = new WidgetMeshModel(nullptr, availableModels);

    m_buttonOk = new QPushButton(tr("Ok"));
    connect(m_buttonOk, &QPushButton::released, this, &DialogAddSceneObject::onButtonOk);
    m_buttonCancel = new QPushButton(tr("Cancel"));
    connect(m_buttonCancel, &QPushButton::released, this, &DialogAddSceneObject::onButtonCancel);

    QHBoxLayout * layoutButtons = new QHBoxLayout();
    layoutButtons->addWidget(m_buttonCancel);
    layoutButtons->addWidget(m_buttonOk);
    QWidget * widgetButtons = new QWidget();
    widgetButtons->setLayout(layoutButtons);

    m_widgetTransform = new WidgetTransform(nullptr, nullptr);

    QVBoxLayout * layoutMain = new QVBoxLayout();
    layoutMain->addWidget(m_widgetMeshModel);
    layoutMain->addWidget(m_widgetTransform);
    layoutMain->addWidget(widgetButtons);

    setLayout(layoutMain);
    setFixedSize(350, 250);
}

std::string DialogAddSceneObject::getSelectedModel() const
{
    return m_pickedModel;
}

Transform DialogAddSceneObject::getTransform() const
{
    return m_widgetTransform->getTransform();
}

void DialogAddSceneObject::onButtonOk()
{
    m_pickedModel = m_widgetMeshModel->getSelectedModel();
    close();
}

void DialogAddSceneObject::onButtonCancel()
{
    close();
}
