#ifndef __WidgetMaterial_hpp__
#define __WidgetMaterial_hpp__

#include <qwidget.h>
#include <qcombobox.h>
#include <qgroupbox.h>
#include <qlayout.h>

#include "core/Materials.hpp"
#include "core/SceneObject.hpp"

#include "WidgetMaterialPBR.hpp"
#include "WidgetMaterialLambert.hpp"

/* A UI widget to represent a material */
class WidgetMaterial : public QWidget
{
    Q_OBJECT
public:
    static const int HEIGHT_PBR = WidgetMaterialPBR::HEIGHT + 70;
    static const int HEIGHT_LAMBERT = WidgetMaterialLambert::HEIGHT + 70;

    WidgetMaterial(QWidget *parent, vengine::ComponentMaterial &materialComponent);

    void updateAvailableMaterials();

private:
    vengine::ComponentMaterial &m_materialComponent;

    QComboBox *m_comboBoxAvailableMaterials;
    QWidget *m_materialWidget = nullptr;

    void createUI(QWidget *widgetMaterial);
    QWidget *createMaterialWidget(std::shared_ptr<vengine::Material> &material);

    QWidget *m_widgetGroupBox = nullptr;
    QVBoxLayout *m_layoutGroupBox = nullptr;
    QVBoxLayout *m_layoutMain = nullptr;
    QWidget *m_widgetMaterial = nullptr;

private Q_SLOTS:
    void onMaterialChanged(int);
};

#endif