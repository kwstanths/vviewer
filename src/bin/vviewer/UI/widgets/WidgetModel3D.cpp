#include "WidgetModel3D.hpp"

#include <iostream>

#include <qgroupbox.h>
#include <qlayout.h>
#include <qlabel.h>

#include "vengine/core/AssetManager.hpp"

#include "UI/UIUtils.hpp"

using namespace vengine;

WidgetModel3D::WidgetModel3D(QWidget *parent, ComponentMesh &meshComponent)
    : QWidget(parent)
    , m_meshComponent(meshComponent)
{
    m_model = meshComponent.mesh->m_model;

    QGroupBox *boxPickModel = new QGroupBox();

    m_models = new QComboBox();
    m_models->addItems(getImportedModels());
    m_models->setCurrentText(QString::fromStdString(m_model->name()));
    connect(m_models, SIGNAL(currentIndexChanged(int)), this, SLOT(onMeshModelChangedSlot(int)));
    QHBoxLayout *layoutPickModel = new QHBoxLayout();
    layoutPickModel->addWidget(new QLabel("Model: "));
    layoutPickModel->addWidget(m_models);
    layoutPickModel->setContentsMargins(0, 0, 0, 0);
    layoutPickModel->setAlignment(Qt::AlignTop);
    QWidget *widgetPickModel = new QWidget();
    widgetPickModel->setLayout(layoutPickModel);

    m_meshes = new QComboBox();
    m_meshes->addItems(getModelMeshes(m_model));
    m_meshes->setCurrentText(QString::fromStdString(meshComponent.mesh->name()));
    connect(m_meshes, SIGNAL(currentIndexChanged(int)), this, SLOT(onMeshChangedSlot(int)));
    QHBoxLayout *layoutPickMesh = new QHBoxLayout();
    layoutPickMesh->addWidget(new QLabel("Mesh: "));
    layoutPickMesh->addWidget(m_meshes);
    layoutPickMesh->setContentsMargins(0, 0, 0, 0);
    layoutPickMesh->setAlignment(Qt::AlignTop);
    QWidget *widgetPickMesh = new QWidget();
    widgetPickMesh->setLayout(layoutPickMesh);

    QVBoxLayout *layoutGroupBox = new QVBoxLayout();
    layoutGroupBox->addWidget(widgetPickModel);
    layoutGroupBox->addWidget(widgetPickMesh);
    boxPickModel->setLayout(layoutGroupBox);

    QVBoxLayout *layoutMain = new QVBoxLayout();
    layoutMain->addWidget(boxPickModel);
    layoutMain->setContentsMargins(0, 0, 0, 0);
    layoutMain->setAlignment(Qt::AlignTop);
    setLayout(layoutMain);
}

uint32_t WidgetModel3D::getHeight() const
{
    return 90;
}

std::string WidgetModel3D::getSelectedModel() const
{
    return m_models->currentText().toStdString();
}

std::string WidgetModel3D::getSelectedMesh() const
{
    return m_meshes->currentText().toStdString();
}

QStringList WidgetModel3D::getModelMeshes(const Model3D *model)
{
    QStringList list;
    for (auto itr : model->meshes()) {
        list.push_back(QString::fromStdString(itr->name()));
    }
    return list;
}

void WidgetModel3D::onMeshModelChangedSlot(int)
{
    std::string newModel = getSelectedModel();

    auto &meshModels = AssetManager::getInstance().modelsMap();
    m_model = meshModels.get(newModel);
    /* Update mesh combo box with the relevant meshes */
    m_meshes->clear();
    m_meshes->addItems(getModelMeshes(m_model));
}

void WidgetModel3D::onMeshChangedSlot(int)
{
    /* Find the mesh from the selected mesh model, and assign it to the scene object */
    std::string selectedMesh = getSelectedMesh();
    for (auto itr : m_model->meshes()) {
        if (itr->name() == selectedMesh) {
            m_meshComponent.mesh = itr;
        }
    }
}
