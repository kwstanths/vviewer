#ifndef __WidgetMaterial_hpp__
#define __WidgetMaterial_hpp__

#include <qwidget.h>
#include <qcombobox.h>
#include <qgroupbox.h>
#include <qlayout.h>

#include "core/Materials.hpp"
#include "core/SceneObject.hpp"

/* A UI widget to represent a material */
class WidgetMaterial : public QWidget
{
    Q_OBJECT
public:
    WidgetMaterial(QWidget* parent, ComponentMaterial& materialComponent);

    void updateAvailableMaterials();
private:
    ComponentMaterial& m_materialComponent;

    QComboBox* m_comboBoxAvailableMaterials;
    QWidget* m_materialWidget = nullptr;

    void createUI(QWidget* widgetMaterial);
    QWidget* createMaterialWidget(std::shared_ptr<Material>& material);

    QGroupBox* m_widgetGroupBox = nullptr;
    QVBoxLayout* m_layoutGroupBox = nullptr;
    QVBoxLayout* m_layoutMain = nullptr;
    QWidget* m_widgetMaterial = nullptr;

private slots:
    void onMaterialChanged(int);
};

#endif