#include "DialogCreateMaterial.hpp"

#include "core/Materials.hpp"
#include "core/AssetManager.hpp"

#include <qlayout.h>
#include <qlabel.h>
#include <qmessagebox.h>

DialogCreateMaterial::DialogCreateMaterial(QWidget *parent, const char *name, QStringList availableModels) 
{
    m_name = new QTextEdit();
    m_name->setPlainText("");
    m_name->setFixedHeight(25);

    QHBoxLayout * layout_name = new QHBoxLayout();
    layout_name->addWidget(new QLabel("Name:"));
    layout_name->addWidget(m_name);
    layout_name->setContentsMargins(0, 0, 0, 0);
    QWidget * widget_name = new QWidget();
    widget_name->setLayout(layout_name);

    m_materialType = new QComboBox();

    QStringList availableMaterials;
    for (size_t m=0; m<materialTypeNames.size(); m++) {
        availableMaterials.push_back(QString::fromStdString(materialTypeNames.find(static_cast<MaterialType>(m))->second));
    }
    m_materialType->addItems(availableMaterials);

    m_buttonOk = new QPushButton(tr("Ok"));
    connect(m_buttonOk, &QPushButton::released, this, &DialogCreateMaterial::onButtonOk);
    m_buttonCancel = new QPushButton(tr("Cancel"));
    connect(m_buttonCancel, &QPushButton::released, this, &DialogCreateMaterial::onButtonCancel);

    QHBoxLayout * layoutButtons = new QHBoxLayout();
    layoutButtons->addWidget(m_buttonCancel);
    layoutButtons->addWidget(m_buttonOk);
    QWidget * widgetButtons = new QWidget();
    widgetButtons->setLayout(layoutButtons);

    QVBoxLayout * layoutMain = new QVBoxLayout();
    layoutMain->addWidget(widget_name);
    layoutMain->addWidget(m_materialType);
    layoutMain->addWidget(widgetButtons);

    setLayout(layoutMain);
    setFixedSize(350, 120);
}

void DialogCreateMaterial::onButtonOk()
{
    std::string chosenName = m_name->toPlainText().toStdString();

    AssetManager<std::string, Material>& instance = AssetManager<std::string, Material>::getInstance();
    if (instance.isPresent(chosenName)) {
        QMessageBox::warning(this, tr("Material already present"),
            tr("This material name already exists"),
            QMessageBox::Ok,
            QMessageBox::Ok);
        return;
    }

    m_selectedName = m_name->toPlainText();
    m_selectedMaterialType = static_cast<MaterialType>(m_materialType->currentIndex());
    close();
}

void DialogCreateMaterial::onButtonCancel()
{
    close();
}
