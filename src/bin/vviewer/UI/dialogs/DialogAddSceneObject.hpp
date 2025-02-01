#ifndef __DialogAddSceneObject_hpp__
#define __DialogAddSceneObject_hpp__

#include <qdialog.h>
#include <qstringlist.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qcheckbox.h>

#include "UI/widgets/WidgetTransform.hpp"

/* A dialog to add an object in a scene */
class DialogAddSceneObject : public QDialog
{
    Q_OBJECT
public:
    DialogAddSceneObject(QWidget *parent, const char *name, QStringList availableModels, QStringList availableMaterials);

    std::string getSelectedModel() const;
    std::optional<vengine::Transform> getOverrideTransform() const;
    std::optional<std::string> getSelectedOverrideMaterial() const;

private:
    QPushButton *m_buttonOk = nullptr;
    QPushButton *m_buttonCancel = nullptr;
    QCheckBox *m_checkBoxTransform = nullptr;
    WidgetTransform *m_widgetTransform = nullptr;
    QCheckBox *m_checkBoxMaterial = nullptr;
    QComboBox *m_comboBoxAvailableMaterials = nullptr;
    QComboBox *m_models = nullptr;

    std::string m_pickedModel = "";
    std::optional<vengine::Transform> m_pickedOverrideTransform;
    std::optional<std::string> m_pickedOverrideMaterial;

private Q_SLOTS:
    void onCheckBoxOverrideTransform(int);
    void onCheckBoxOverrideMaterial(int);
    void onButtonOk();
    void onButtonCancel();
};

#endif