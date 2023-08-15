#include "WidgetRightPanel.hpp"

#include <qaction.h>
#include <qboxlayout.h>
#include <qnamespace.h>
#include <qscrollarea.h>
#include <qpushbutton.h>
#include <qwidget.h>
#include <qmenu.h>

#include <vulkan/vulkan_core.h>

#include "UI/widgets/WidgetComponent.hpp"
#include "UI/widgets/WidgetEnvironment.hpp"
#include "UI/widgets/WidgetMaterial.hpp"
#include "utils/ECS.hpp"
#include "core/AssetManager.hpp"

WidgetRightPanel::WidgetRightPanel(QWidget * parent, Engine * engine): QWidget(parent), m_engine(engine)
{
    m_layoutControls = new QVBoxLayout();
    m_layoutControls->setAlignment(Qt::AlignTop);
    m_widgetControls = new QWidget();
    m_widgetControls->setLayout(m_layoutControls);
    m_widgetControls->setFixedWidth(350);

    m_widgetScroll = new QScrollArea();
    m_widgetScroll->setWidget(m_widgetControls);
    
    m_widgetEnvironment = new WidgetEnvironment(nullptr, m_engine->scene());

    QTabWidget* widgetTab = new QTabWidget();
    widgetTab->insertTab(0, m_widgetScroll, "Scene object");
    widgetTab->insertTab(1, m_widgetEnvironment, "Environment");

    QHBoxLayout * layoutMain = new QHBoxLayout();
    layoutMain->addWidget(widgetTab);

    setLayout(layoutMain);

    m_updateTimer = new QTimer();
    m_updateTimer->setInterval(30);
    connect(m_updateTimer, SIGNAL(timeout()), this, SLOT(onUpdate()));
    m_updateTimer->start();
}

void WidgetRightPanel::setSelectedObject(std::shared_ptr<SceneObject> object)
{
    m_object = object;

    deleteWidgets();
    createUI(object);
}

WidgetEnvironment* WidgetRightPanel::getEnvironmentWidget()
{
    return m_widgetEnvironment;
}

void WidgetRightPanel::updateAvailableMaterials()
{
    if (m_selectedObjectWidgetMaterial != nullptr) {
        m_selectedObjectWidgetMaterial->getWidget<WidgetMaterial>()->updateAvailableMaterials();
    }
}

void WidgetRightPanel::onUpdate()
{
    if (m_selectedObjectWidgetMaterial != nullptr) {
        m_selectedObjectWidgetMaterial->updateHeight();
    }
}

void WidgetRightPanel::deleteWidgets()
{
    delete m_layoutControls;
    delete m_widgetControls;

    m_selectedObjectWidgetName = nullptr;
    m_selectedObjectWidgetTransform = nullptr;
    m_selectedObjectWidgetMeshModel = nullptr;
    m_selectedObjectWidgetMaterial = nullptr;
    m_selectedObjectWidgetLight = nullptr;
}

void WidgetRightPanel::createUI(std::shared_ptr<SceneObject> object)
{
    m_layoutControls = new QVBoxLayout();
    m_layoutControls->setAlignment(Qt::AlignTop);
    m_layoutControls->setSpacing(10);

    m_selectedObjectWidgetName = new WidgetName(nullptr, QString(object->m_name.c_str()));
    connect(m_selectedObjectWidgetName->m_text, &QTextEdit::textChanged, this, &WidgetRightPanel::onSceneObjectNameChanged);
    m_layoutControls->addWidget(m_selectedObjectWidgetName);

    m_selectedObjectWidgetTransform = new WidgetTransform(nullptr, object, "Transform", true);
    m_layoutControls->addWidget(m_selectedObjectWidgetTransform);

    if (object->has<ComponentMesh>())
    {
        m_selectedObjectWidgetMeshModel = new WidgetComponent(nullptr, new UIComponentMesh(object, "Mesh Model"), m_engine);
        connect(m_selectedObjectWidgetMeshModel, &WidgetComponent::componentRemoved, this, &WidgetRightPanel::onComponentRemoved);
        m_layoutControls->addWidget(m_selectedObjectWidgetMeshModel);
    }
    
    if (object->has<ComponentMaterial>()) 
    {
        m_selectedObjectWidgetMaterial = new WidgetComponent(nullptr, new UIComponentMaterial(object, "Material"), m_engine);
        connect(m_selectedObjectWidgetMaterial, &WidgetComponent::componentRemoved, this, &WidgetRightPanel::onComponentRemoved);
        m_layoutControls->addWidget(m_selectedObjectWidgetMaterial);
    }

    if (object->has<ComponentLight>()) 
    {
        m_selectedObjectWidgetLight = new WidgetComponent(nullptr, new UIComponentLight(object, "Light"), m_engine);
        connect(m_selectedObjectWidgetLight, &WidgetComponent::componentRemoved, this, &WidgetRightPanel::onComponentRemoved);
        m_layoutControls->addWidget(m_selectedObjectWidgetLight);
    }

    QMenu *menu = new QMenu();
    if (!object->has<ComponentMesh>()) {
        QAction * actionAddMeshComponent = new QAction("Mesh", this);
        connect(actionAddMeshComponent, SIGNAL(triggered()), this, SLOT(onAddComponentMesh()));
        menu->addAction(actionAddMeshComponent);
    }
    if (!object->has<ComponentMaterial>()) {
        QAction * actionAddMaterialComponent = new QAction("Material", this);
        connect(actionAddMaterialComponent, SIGNAL(triggered()), this, SLOT(onAddComponentMaterial()));
        menu->addAction(actionAddMaterialComponent);
    }
    if (!object->has<ComponentLight>()) {
        QAction * actionAddLightComponent = new QAction("Light", this);
        connect(actionAddLightComponent, SIGNAL(triggered()), this, SLOT(onAddComponentLight()));
        menu->addAction(actionAddLightComponent);
    }
    QPushButton * buttonAddComponent = new QPushButton(tr("Add component"));
    buttonAddComponent->setMenu(menu);
    m_layoutControls->addWidget(buttonAddComponent);

    m_widgetControls = new QWidget();
    m_widgetControls->setLayout(m_layoutControls);
    m_widgetScroll->setWidget(m_widgetControls);
    m_widgetControls->setFixedWidth(340);
}

void WidgetRightPanel::onTransformChanged()
{
    m_selectedObjectWidgetTransform->updateTransformUI();
}

void WidgetRightPanel::onSceneObjectNameChanged()
{
    QString newName = m_selectedObjectWidgetName->m_text->toPlainText();
    Q_EMIT selectedSceneObjectNameChanged(newName);
}

void WidgetRightPanel::onComponentRemoved()
{
    deleteWidgets();
    createUI(m_object);
}

void WidgetRightPanel::onAddComponentMesh()
{
    AssetManager<std::string, MeshModel>& instanceModels = AssetManager<std::string, MeshModel>::getInstance();
    auto sphere = instanceModels.get("assets/models/uvsphere.obj");

    m_engine->stop();
    m_engine->waitIdle();

    m_object->add<ComponentMesh>().mesh = sphere->getMeshes()[0];

    m_engine->start();

    deleteWidgets();
    createUI(m_object);
}

void WidgetRightPanel::onAddComponentMaterial()
{
    AssetManager<std::string, Material>& instanceMaterials = AssetManager<std::string, Material>::getInstance();
    auto matDef = instanceMaterials.get("defaultMaterial");

    m_engine->stop();
    m_engine->waitIdle();

    m_object->add<ComponentMaterial>().material = matDef;

    m_engine->start();

    deleteWidgets();
    createUI(m_object);
}

void WidgetRightPanel::onAddComponentLight()
{
    AssetManager<std::string, LightMaterial>& instanceLightMaterials = AssetManager<std::string, LightMaterial>::getInstance();
    auto pointLightMaterial = instanceLightMaterials.get("DefaultPointLightMaterial");

    m_engine->stop();
    m_engine->waitIdle();

    m_object->add<ComponentLight>().light = std::make_shared<Light>("PointLight", LightType::POINT_LIGHT, pointLightMaterial);

    m_engine->start();

    deleteWidgets();
    createUI(m_object);
}

