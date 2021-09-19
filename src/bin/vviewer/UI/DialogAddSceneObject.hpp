#ifndef __DialogAddSceneObject_hpp__
#define __DialogAddSceneObject_hpp__

#include <qdialog.h>
#include <qstringlist.h>
#include <qcombobox.h>
#include <qpushbutton.h>

#include "WidgetTransform.hpp"

class DialogAddSceneObject : public QDialog
{
    Q_OBJECT
public:
    DialogAddSceneObject(QWidget *parent, const char *name, QStringList availableModels);

    std::string getSelectedModel() const;
    Transform getTransform() const;

private:
    QComboBox * m_models = nullptr;
    QPushButton * m_buttonOk = nullptr;
    QPushButton * m_buttonCancel = nullptr;
    WidgetTransform * m_widgetTransform = nullptr;

    std::string m_pickedModel = "";


private slots:
    void onButtonOk();
    void onButtonCancel();
};

#endif