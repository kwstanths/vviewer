#include "WidgetMeshModel.hpp"

#include <iostream>

#include <qgroupbox.h>
#include <qlayout.h>
#include <qlabel.h>

#include "core/AssetManager.hpp"

#include "UIUtils.hpp"

WidgetMeshModel::WidgetMeshModel(QWidget * parent, ComponentMesh& meshComponent) : QWidget(parent), m_meshComponent(meshComponent)
{
    m_meshModel = meshComponent.mesh->m_meshModel;

    QGroupBox * boxPickModel = new QGroupBox(tr("Mesh Model"));

    m_models = new QComboBox();
    m_models->addItems(getImportedModels());
    m_models->setCurrentText(QString::fromStdString(m_meshModel->getName()));
    connect(m_models, SIGNAL(currentIndexChanged(int)), this, SLOT(onMeshModelChangedSlot(int)));    
    QHBoxLayout * layoutPickModel = new QHBoxLayout();
    layoutPickModel->addWidget(new QLabel("Model: "));
    layoutPickModel->addWidget(m_models);
    layoutPickModel->setContentsMargins(0, 0, 0, 0);
    layoutPickModel->setAlignment(Qt::AlignTop);
    QWidget * widgetPickModel = new QWidget();
    widgetPickModel->setLayout(layoutPickModel);

    m_meshes = new QComboBox();
    m_meshes->addItems(getModelMeshes(m_meshModel));
    m_meshes->setCurrentText(QString::fromStdString(meshComponent.mesh->m_name));
    connect(m_meshes, SIGNAL(currentIndexChanged(int)), this, SLOT(onMeshChangedSlot(int)));    
    QHBoxLayout * layoutPickMesh = new QHBoxLayout();
    layoutPickMesh->addWidget(new QLabel("Mesh: "));
    layoutPickMesh->addWidget(m_meshes);
    layoutPickMesh->setContentsMargins(0, 0, 0, 0);
    layoutPickMesh->setAlignment(Qt::AlignTop);
    QWidget * widgetPickMesh = new QWidget();
    widgetPickMesh->setLayout(layoutPickMesh);

    QVBoxLayout * layoutGroupBox = new QVBoxLayout();
    layoutGroupBox->addWidget(widgetPickModel);
    layoutGroupBox->addWidget(widgetPickMesh);
    boxPickModel->setLayout(layoutGroupBox);
    
    QVBoxLayout * layoutMain = new QVBoxLayout();
    layoutMain->addWidget(boxPickModel);
    layoutMain->setContentsMargins(0, 0, 0, 0);
    layoutMain->setAlignment(Qt::AlignTop);
    setLayout(layoutMain);
    setFixedHeight(90);
}

std::string WidgetMeshModel::getSelectedModel() const
{
    return m_models->currentText().toStdString();
}

std::string WidgetMeshModel::getSelectedMesh() const
{
    return m_meshes->currentText().toStdString();
}

QStringList WidgetMeshModel::getModelMeshes(const MeshModel* model)
{
    QStringList list;
    for(auto itr : model->getMeshes())
    {
        list.push_back(QString::fromStdString(itr->m_name));
    }
    return list;
}

void WidgetMeshModel::onMeshModelChangedSlot(int)
{
    std::string newModel = getSelectedModel();

    AssetManager<std::string, MeshModel>& instance = AssetManager<std::string, MeshModel>::getInstance();
    m_meshModel = instance.get(newModel).get();
    /* Update mesh combo box with the relevant meshes */
    m_meshes->clear();
    m_meshes->addItems(getModelMeshes(m_meshModel));
}

void WidgetMeshModel::onMeshChangedSlot(int)
{
    /* Find the mesh from the selected mesh model, and assign it to the scene object */
    std::string selectedMesh = getSelectedMesh();
    for(auto itr : m_meshModel->getMeshes())
    {
        if (itr->m_name == selectedMesh)
        {
            m_meshComponent.mesh = itr;
        }
    }
}
