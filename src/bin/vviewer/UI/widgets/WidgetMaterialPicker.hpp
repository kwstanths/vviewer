#ifndef __WidgetMaterialPicker_hpp__
#define __WidgetMaterialPicker_hpp__

#include <qwidget.h>
#include <qspinbox.h>
#include <qcombobox.h>
#include <qlayout.h>

#include "vengine/core/Materials.hpp"

class WidgetMaterialPicker : public QWidget
{
    Q_OBJECT
public:
    WidgetMaterialPicker(QWidget *parent, vengine::MaterialType materialType, QString groupBoxName = "");

    void updateAvailableMaterials();
    void setMaterial(vengine::Material *material);
    int getWidgetHeight() const;

private:
    QComboBox *m_comboBoxAvailableMaterials = nullptr;
    QWidget *m_materialWidget = nullptr;
    vengine::MaterialType m_materialType;
    vengine::Material *m_material = nullptr;

    QWidget *m_widgetGroupBox = nullptr;
    QVBoxLayout *m_layoutGroupBox = nullptr;
    QVBoxLayout *m_layoutMain = nullptr;

    QWidget *createMaterialWidget(vengine::Material *material);
    void createUI(QWidget *widgetMaterial);

public Q_SLOTS:
    void onMaterialChanged(int);

Q_SIGNALS:
    void materialChanged(vengine::Material *material);
};

#endif
