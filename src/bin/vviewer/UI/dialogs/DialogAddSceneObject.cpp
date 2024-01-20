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

    m_checkBoxTransform = new QCheckBox("Override Transform:");
    connect(m_checkBoxTransform, SIGNAL(stateChanged(int)), this, SLOT(onCheckBoxOverrideTransform(int)));
    m_widgetTransform = new WidgetTransform(nullptr, nullptr);
    m_widgetTransform->setEnabled(false);

    m_comboBoxAvailableMaterials = new QComboBox();
    m_comboBoxAvailableMaterials->addItems(availableMaterials);
    m_comboBoxAvailableMaterials->setEnabled(false);

    QGroupBox *groupBoxOverrideMaterial = new QGroupBox(tr("Material"));
    m_checkBoxMaterial = new QCheckBox("Override material:");
    connect(m_checkBoxMaterial, SIGNAL(stateChanged(int)), this, SLOT(onCheckBoxOverrideMaterial(int)));

    QVBoxLayout *layoutMaterials = new QVBoxLayout();
    layoutMaterials->addWidget(m_checkBoxMaterial);
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
    layoutMain->addWidget(m_checkBoxTransform);
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

std::optional<Transform> DialogAddSceneObject::getOverrideTransform() const
{
    return m_pickedOverrideTransform;
}

std::optional<std::string> DialogAddSceneObject::getSelectedOverrideMaterial() const
{
    return m_pickedOverrideMaterial;
}

void DialogAddSceneObject::onCheckBoxOverrideTransform(int)
{
    if (m_checkBoxTransform->isChecked()) {
        m_widgetTransform->setEnabled(true);
    } else {
        m_widgetTransform->setEnabled(false);
    }
}

void DialogAddSceneObject::onCheckBoxOverrideMaterial(int)
{
    if (m_checkBoxMaterial->isChecked()) {
        m_comboBoxAvailableMaterials->setEnabled(true);
    } else {
        m_comboBoxAvailableMaterials->setEnabled(false);
    }
}

void DialogAddSceneObject::onButtonOk()
{
    m_pickedModel = m_models->currentText().toStdString();
    if (m_checkBoxMaterial->isChecked()) {
        m_pickedOverrideMaterial = m_comboBoxAvailableMaterials->currentText().toStdString();
    }
    if (m_checkBoxTransform->isChecked()) {
        m_pickedOverrideTransform = m_widgetTransform->getTransform();
    }
    close();
}

void DialogAddSceneObject::onButtonCancel()
{
    close();
}
