#ifndef __DialogAddSceneObject_hpp__
#define __DialogAddSceneObject_hpp__

#include <qdialog.h>
#include <qstringlist.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qcheckbox.h>

#include "UI/widgets/WidgetTransform.hpp"
#include "UI/widgets//WidgetModel3D.hpp"

/* A dialog to add an object in a scene */
class DialogAddSceneObject : public QDialog
{
    Q_OBJECT
public:
    DialogAddSceneObject(QWidget *parent, const char *name, QStringList availableModels, QStringList availableMaterials);

    std::string getSelectedModel() const;
    vengine::Transform getTransform() const;
    std::optional<std::string> getSelectedOverrideMaterial() const;

private:
    QPushButton *m_buttonOk = nullptr;
    QPushButton *m_buttonCancel = nullptr;
    WidgetTransform *m_widgetTransform = nullptr;
    QCheckBox *m_checkBox = nullptr;
    QComboBox *m_comboBoxAvailableMaterials = nullptr;
    QComboBox *m_models = nullptr;

    std::string m_pickedModel = "";
    std::optional<std::string> m_pickedOverrideMaterial;

private Q_SLOTS:
    void onCheckBoxOverride(int);
    void onButtonOk();
    void onButtonCancel();
};

#endif