#include "WidgetMeshModel.hpp"

#include <qgroupbox.h>
#include <qlayout.h>

WidgetMeshModel::WidgetMeshModel(QWidget * parent, QStringList availableModels) : QWidget(parent)
{
    m_models = new QComboBox();
    m_models->addItems(availableModels);

    QGroupBox * boxPickModel = new QGroupBox(tr("Mesh Model"));
    QVBoxLayout * layoutPickModel = new QVBoxLayout();
    layoutPickModel->addWidget(m_models);
    layoutPickModel->setContentsMargins(5, 5, 5, 5);
    layoutPickModel->setAlignment(Qt::AlignTop);

    boxPickModel->setLayout(layoutPickModel);
    
    QVBoxLayout * layoutMain = new QVBoxLayout();
    layoutMain->addWidget(boxPickModel);
    layoutMain->setContentsMargins(0, 0, 0, 0);
    layoutMain->setAlignment(Qt::AlignTop);
    setLayout(layoutMain);
    setFixedHeight(55);
}

std::string WidgetMeshModel::getSelectedModel() const
{
    return m_models->currentText().toStdString();
}
