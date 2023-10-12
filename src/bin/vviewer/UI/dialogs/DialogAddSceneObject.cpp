#include "DialogAddSceneObject.hpp"

#include <qlayout.h>
#include <qgroupbox.h>
#include <qlabel.h>

using namespace vengine;

DialogAddSceneObject::DialogAddSceneObject(QWidget *parent,
                                           const char *name,
                                           QStringList availableModels,
                                           QStringList availableMaterials)
    : QDialog(parent)
{
    setWindowTitle(QString(name));

    QGroupBox *boxPickModel = new QGroupBox(tr("Mesh Model"));
    m_models = new QComboBox();
    m_models->addItems(availableModels);
    QHBoxLayout *layoutPickModel = new QHBoxLayout();
    layoutPickModel->addWidget(m_models);
    layoutPickModel->setContentsMargins(5, 5, 5, 5);
    layoutPickModel->setAlignment(Qt::AlignTop);
    boxPickModel->setLayout(layoutPickModel);

    m_widgetTransform = new WidgetTransform(nullptr, nullptr);

    m_comboBoxAvailableMaterials = new QComboBox();
    m_comboBoxAvailableMaterials->addItems(availableMaterials);
    m_comboBoxAvailableMaterials->setEnabled(false);

    QGroupBox *groupBoxOverrideMaterial = new QGroupBox(tr("Material"));
    m_checkBox = new QCheckBox("Override material:");
    connect(m_checkBox, SIGNAL(stateChanged(int)), this, SLOT(onCheckBoxOverride(int)));

    QVBoxLayout *layoutMaterials = new QVBoxLayout();
    layoutMaterials->addWidget(m_checkBox);
    layoutMaterials->addWidget(m_comboBoxAvailableMaterials);
    layoutMaterials->setContentsMargins(5, 5, 5, 5);
    groupBoxOverrideMaterial->setLayout(layoutMaterials);

    m_buttonOk = new QPushButton(tr("Ok"));
    connect(m_buttonOk, &QPushButton::released, this, &DialogAddSceneObject::onButtonOk);
    m_buttonCancel = new QPushButton(tr("Cancel"));
    connect(m_buttonCancel, &QPushButton::released, this, &DialogAddSceneObject::onButtonCancel);

    QHBoxLayout *layoutButtons = new QHBoxLayout();
    layoutButtons->addWidget(m_buttonCancel);
    layoutButtons->addWidget(m_buttonOk);
    QWidget *widgetButtons = new QWidget();
    widgetButtons->setLayout(layoutButtons);

    QVBoxLayout *layoutMain = new QVBoxLayout();
    layoutMain->addWidget(boxPickModel);
    layoutMain->addWidget(m_widgetTransform);
    layoutMain->addWidget(groupBoxOverrideMaterial);
    layoutMain->addWidget(widgetButtons);

    setLayout(layoutMain);
    setFixedSize(350, 360);
}

std::string DialogAddSceneObject::getSelectedModel() const
{
    return m_pickedModel;
}

Transform DialogAddSceneObject::getTransform() const
{
    return m_widgetTransform->getTransform();
}

std::optional<std::string> DialogAddSceneObject::getSelectedOverrideMaterial() const
{
    return m_pickedOverrideMaterial;
}

void DialogAddSceneObject::onCheckBoxOverride(int)
{
    if (m_checkBox->isChecked()) {
        m_comboBoxAvailableMaterials->setEnabled(true);
    } else {
        m_comboBoxAvailableMaterials->setEnabled(false);
    }
}

void DialogAddSceneObject::onButtonOk()
{
    m_pickedModel = m_models->currentText().toStdString();
    if (m_checkBox->isChecked()) {
        m_pickedOverrideMaterial = m_comboBoxAvailableMaterials->currentText().toStdString();
    }
    close();
}

void DialogAddSceneObject::onButtonCancel()
{
    close();
}
