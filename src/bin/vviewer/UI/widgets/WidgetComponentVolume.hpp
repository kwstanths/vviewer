#ifndef __WidgetComponentVolume_hpp__
#define __WidgetComponentVolume_hpp__

#include <qwidget.h>
#include <qcombobox.h>
#include <qgroupbox.h>
#include <qlayout.h>
#include <qlabel.h>

#include "vengine/core/Materials.hpp"
#include "vengine/core/SceneObject.hpp"

#include "WidgetMaterialPicker.hpp"

/* A UI widget to represent a volume component */
class WidgetComponentVolume : public QWidget
{
    Q_OBJECT
public:
    WidgetComponentVolume(QWidget *parent, vengine::ComponentVolume &volumeComponent);

    void updateAvailableMaterials();
    int getWidgetHeight();

private:
    vengine::ComponentVolume &m_volumeComponent;

    WidgetMaterialPicker *m_materialPickerFront = nullptr;
    WidgetMaterialPicker *m_materialPickerBack = nullptr;

    void createUI();

    QWidget *m_widgetGroupBox = nullptr;
    QVBoxLayout *m_layoutGroupBox = nullptr;
    QVBoxLayout *m_layoutMain = nullptr;
    QWidget *m_widgetMaterial = nullptr;

private Q_SLOTS:
    void onMaterialChangedFrontFacing(vengine::Material *material);
    void onMaterialChangedBackFacing(vengine::Material *material);
};

#endif