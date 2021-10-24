#ifndef __DialogCreateMaterial_hpp__
#define __DialogCreateMaterial_hpp__

#include <qdialog.h>
#include <qstringlist.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qtextedit.h>

#include "core/Materials.hpp"

/* A dialog to add an object in a scene */
class DialogCreateMaterial : public QDialog
{
    Q_OBJECT
public:
    DialogCreateMaterial(QWidget *parent, const char *name, QStringList availableModels);

    MaterialType m_selectedMaterial = MaterialType::MATERIAL_NOT_SET;
    QString m_selectedName = "";

private:
    QTextEdit * m_name;
    QComboBox * m_materialType;
    QPushButton * m_buttonOk = nullptr;
    QPushButton * m_buttonCancel = nullptr;

private slots:
    void onButtonOk();
    void onButtonCancel();
};

#endif