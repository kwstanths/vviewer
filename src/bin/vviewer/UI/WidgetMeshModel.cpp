#include "WidgetMeshModel.hpp"

#include <qgroupbox.h>
#include <qlayout.h>

#include "core/AssetManager.hpp"

WidgetMeshModel::WidgetMeshModel(QWidget * parent, SceneObject * sceneObject, QStringList availableModels) : QWidget(parent)
{
    m_sceneObject = sceneObject;

    m_models = new QComboBox();
    m_models->addItems(availableModels);
    if (sceneObject != nullptr) {
        m_models->setCurrentText(QString::fromStdString(sceneObject->get<Mesh*>(ComponentType::MESH)->m_name));
    }
    connect(m_models, SIGNAL(currentIndexChanged(int)), this, SLOT(onMeshModelChangedSlot(int)));

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

void WidgetMeshModel::onMeshModelChangedSlot(int)
{
    if (m_sceneObject == nullptr) return;

    /* Selected object name widget changed */
    std::string newModel = getSelectedModel();

    AssetManager<std::string, MeshModel *>& instance = AssetManager<std::string, MeshModel *>::getInstance();
    //m_sceneObject->setMesh(instance.Get(newModel));
}