#ifndef __WidgetComponent_hpp__
#define __WidgetComponent_hpp__

#include <memory>
#include <qwidget.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qgroupbox.h>
#include <stdexcept>

#include "UI/widgets/WidgetLight.hpp"
#include "vengine/core/Materials.hpp"
#include "vengine/core/SceneObject.hpp"
#include "vengine/core/Engine.hpp"
#include "vengine/utils/ECS.hpp"

#include "WidgetModel3D.hpp"
#include "WidgetMaterial.hpp"

/* UI classes that represent each available component, QT doesn't support templated Q_OBJECT */
class UIComponentWrapper
{
public:
    UIComponentWrapper(vengine::SceneObject *object, QString name)
        : m_object(object)
        , m_name(name){};
    virtual ~UIComponentWrapper(){};

    virtual QWidget *generateWidget() = 0;
    virtual void removeComponent() = 0;
    virtual int getWidgetHeight() = 0;

    QString m_name;

protected:
    vengine::SceneObject *m_object;
};

/* A UI widget to represent a specific component */
class WidgetComponent : public QWidget
{
    Q_OBJECT
public:
    WidgetComponent(QWidget *parent, UIComponentWrapper *componentWrapper, vengine::Engine *engine);
    ~WidgetComponent();

    template <typename T>
    T *getWidget()
    {
        return static_cast<T *>(m_widgetEncapsulated);
    }

    void updateHeight();

private:
    QWidget *m_widgetEncapsulated;
    UIComponentWrapper *m_componentWrapper;
    vengine::Engine *m_engine;

private Q_SLOTS:
    void onRemoved();

Q_SIGNALS:
    void componentRemoved();
};

class UIComponentMesh : public UIComponentWrapper
{
public:
    UIComponentMesh(vengine::SceneObject *object, QString name)
        : UIComponentWrapper(object, name){};

    QWidget *generateWidget() { return new WidgetModel3D(nullptr, m_object->get<vengine::ComponentMesh>()); }

    void removeComponent()
    {
        m_object->remove<vengine::ComponentMesh>();
        return;
    }

    int getWidgetHeight() { return WidgetModel3D::HEIGHT; }

private:
};

class UIComponentMaterial : public UIComponentWrapper
{
public:
    UIComponentMaterial(vengine::SceneObject *object, QString name)
        : UIComponentWrapper(object, name){};

    QWidget *generateWidget() { return new WidgetMaterial(nullptr, m_object->get<vengine::ComponentMaterial>()); }

    void removeComponent()
    {
        m_object->remove<vengine::ComponentMaterial>();
        return;
    }

    int getWidgetHeight()
    {
        if (m_object->get<vengine::ComponentMaterial>().material->type() == vengine::MaterialType::MATERIAL_PBR_STANDARD) {
            return WidgetMaterial::HEIGHT_PBR;
        } else if (m_object->get<vengine::ComponentMaterial>().material->type() == vengine::MaterialType::MATERIAL_LAMBERT) {
            return WidgetMaterial::HEIGHT_LAMBERT;
        } else if (m_object->get<vengine::ComponentMaterial>().material->type() == vengine::MaterialType::MATERIAL_VOLUME) {
            return WidgetMaterial::HEIGHT_VOLUME;
        } else {
            throw std::runtime_error("UIComponentMaterial::Unexpected material");
        }
    }

private:
};

class UIComponentLight : public UIComponentWrapper
{
public:
    UIComponentLight(vengine::SceneObject *object, QString name)
        : UIComponentWrapper(object, name){};

    QWidget *generateWidget() { return new WidgetLight(nullptr, m_object->get<vengine::ComponentLight>()); }

    void removeComponent()
    {
        m_object->remove<vengine::ComponentLight>();
        return;
    }

    int getWidgetHeight() { return WidgetLight::HEIGHT; }

private:
};

#endif
