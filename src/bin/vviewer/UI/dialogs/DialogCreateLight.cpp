#include "DialogCreateLight.hpp"

#include "vengine/core/Materials.hpp"
#include "vengine/core/AssetManager.hpp"

#include <qlayout.h>
#include <qlabel.h>
#include <qmessagebox.h>

using namespace vengine;

DialogCreateLight::DialogCreateLight(QWidget *parent, const char *name)
{
    m_name = new QTextEdit();
    m_name->setPlainText("");
    m_name->setFixedHeight(25);

    QHBoxLayout *layout_name = new QHBoxLayout();
    layout_name->addWidget(new QLabel("Name:"));
    layout_name->addWidget(m_name);
    layout_name->setContentsMargins(0, 0, 0, 0);
    QWidget *widget_name = new QWidget();
    widget_name->setLayout(layout_name);

    m_lightType = new QComboBox();

    QStringList availableLights;
    for (size_t m = 0; m < lightTypeNames.size(); m++) {
        availableLights.push_back(QString::fromStdString(lightTypeNames.find(static_cast<LightType>(m))->second));
    }
    m_lightType->addItems(availableLights);

    m_buttonOk = new QPushButton(tr("Ok"));
    connect(m_buttonOk, &QPushButton::released, this, &DialogCreateLight::onButtonOk);
    m_buttonCancel = new QPushButton(tr("Cancel"));
    connect(m_buttonCancel, &QPushButton::released, this, &DialogCreateLight::onButtonCancel);

    QHBoxLayout *layoutButtons = new QHBoxLayout();
    layoutButtons->addWidget(m_buttonCancel);
    layoutButtons->addWidget(m_buttonOk);
    QWidget *widgetButtons = new QWidget();
    widgetButtons->setLayout(layoutButtons);

    QVBoxLayout *layoutMain = new QVBoxLayout();
    layoutMain->addWidget(widget_name);
    layoutMain->addWidget(m_lightType);
    layoutMain->addWidget(widgetButtons);

    setLayout(layoutMain);
    setFixedSize(350, 120);
}

void DialogCreateLight::onButtonOk()
{
    std::string chosenName = m_name->toPlainText().toStdString();

    auto &materials = AssetManager::getInstance().materialsMap();
    if (materials.has(chosenName)) {
        QMessageBox::warning(
            this, tr("Material already present"), tr("This material name already exists"), QMessageBox::Ok, QMessageBox::Ok);
        return;
    }

    m_selectedName = m_name->toPlainText();
    m_selectedLightType = static_cast<LightType>(m_lightType->currentIndex());
    close();
}

void DialogCreateLight::onButtonCancel()
{
    close();
}
